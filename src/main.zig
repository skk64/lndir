const std = @import("std");
const Dir = std.fs.Dir;
const Allocator = std.mem.Allocator;
const assert = std.debug.assert;

pub fn help(bin_name: [*:0]const u8) !void {
    const help_message: []const u8 =
        \\ lndir
        \\Usage: {0s} <source_directory> <target_directory>
        \\Recursively create hard links for all files in <source_directory> within <target_directory>.
        \\Will create duplicate directories in <target_directory>.
        \\
        \\Options:
        \\-h, --help        Show this help message and exit
        \\
        \\Example:
        \\{0s} /path/to/source /path/to/target
        \\{0s} relative/source relative/target
        \\
    ;
    const stdout_file = std.io.getStdOut().writer();
    var bw = std.io.bufferedWriter(stdout_file);
    const stdout = bw.writer();
    try stdout.print(help_message, .{bin_name});
    try bw.flush();
}

const Args = struct {
    source_path: ?[*:0]const u8 = null,
    destination_path: ?[*:0]const u8 = null,
    help: bool = false,

    fn parse(argv: [][*:0]u8) Args {
        var args = Args{};
        for (1..argv.len) |i| {
            const arg = std.mem.span(argv[i]);
            if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
                args.help = true;
            } else {
                if (args.source_path == null) {
                    args.source_path = argv[i];
                } else if (args.destination_path == null) {
                    args.destination_path = argv[i];
                }
            }
        }
        return args;
    }
};

const a = error.Failure{
    .F,
};

pub fn main() !void {
    const argv = std.os.argv;
    const args = Args.parse(argv);

    if (args.help or args.source_path == null or args.destination_path == null) {
        try help(argv[0]);
        std.process.exit(1);
    }

    ln_all_files_in_dir(std.fs.cwd(), args.source_path.?, args.destination_path.?) catch |err| {
        std.debug.print("{any}\n\n", .{err});
        try help(argv[0]);
        std.process.exit(1);
    };
}

fn ln_all_files_in_dir(dir: Dir, source_path: [*:0]const u8, destination_path: [*:0]const u8) !void {
    const destination_path_span: []const u8 = std.mem.span(destination_path);
    const source_path_span: []const u8 = std.mem.span(source_path);

    const source = dir.openDirZ(source_path, .{ .iterate = true, .no_follow = true }) catch |err| {
        std.debug.print("Error opening {s}\n", .{source_path});
        return err;
    };
    var source_path_buf: [std.posix.PATH_MAX]u8 = undefined;
    var file_buf: [std.posix.PATH_MAX]u8 = undefined;
    const source_path_absolute = try source.realpath(".", &source_path_buf);

    // Create destination directory if it doesn't already exist
    dir.makeDirZ(destination_path) catch |err| switch (err) {
        error.PathAlreadyExists => {},
        else => {
            std.debug.print("Could not create destination '{s}'\nExiting\n", .{destination_path});
            return err;
        },
    };
    const destination = dir.openDirZ(destination_path, .{ .iterate = true, .no_follow = true }) catch |err| {
        std.debug.print("Error opening {s}\n", .{destination_path});
        return err;
    };

    // Need memory for walker and file path buffers
    // Walker uses bounded amount of memory, defined by maximum path lengths
    // so FixedBufferAllocator is suitable
    // Maximum memory requirement determined by getdents64() syscall (max of u16) plus 2 paths (and some extra for allocator)
    var mem_buffer: [std.posix.PATH_MAX * 2 + std.math.maxInt(u16) + 0x1000]u8 = undefined;
    var fba = std.heap.FixedBufferAllocator.init(&mem_buffer);
    const allocator = fba.allocator();

    var file_count: u64 = 0;

    // Ansi control codes for progress tracking on same line
    const csi: *const [2:0]u8 = &.{ std.ascii.control_code.esc, '[' };
    const move_cursor_prev_line = csi ++ "F";
    const erase_line = csi ++ "K";
    const overwrite_prev_line = move_cursor_prev_line ++ erase_line;

    const stderr_writer = std.io.getStdErr().writer();
    var bw = std.io.bufferedWriter(stderr_writer);
    const stderr = bw.writer();
    // newline to compensate for first cursor_prev_line code
    try stderr.print("\n", .{});

    var walker = try source.walk(allocator);
    defer walker.deinit();
    while (walker.next() catch |e| blk: {
        std.debug.print("Error accessing files: {any}\n\n", .{e});
        break :blk null;
    }) |p| {
        try bw.flush();

        // skip anything that would cause a recursive loop. (that would cause a file to be linked more than once)
        var file_path_absolute = try destination.realpath(".", &file_buf);
        file_buf[file_path_absolute.len] = '/';
        @memcpy(file_buf[file_path_absolute.len + 1 .. file_path_absolute.len + 1 + p.path.len], p.path);
        file_path_absolute.len += p.path.len + 1;
        if (std.mem.startsWith(u8, file_path_absolute, source_path_absolute)) {
            try stderr.print(overwrite_prev_line ++ "Cannot create link inside source directory. Skipping {s}\n\n", .{p.path});
            continue;
        }
        switch (p.kind) {
            .file, .sym_link => {
                const source_file = try std.fs.path.joinZ(allocator, &.{ source_path_span, p.path });
                const destination_file = try std.fs.path.joinZ(allocator, &.{ destination_path_span, p.path });
                if (std.posix.link(source_file, destination_file)) {
                    file_count += 1;
                    try stderr.print(overwrite_prev_line ++ "Linking  {d} files\n", .{file_count});
                } else |err| {
                    try stderr.print(overwrite_prev_line ++ "Error creating link '{s}': {any}\n\n", .{ destination_file, err });
                }
                allocator.free(destination_file);
                allocator.free(source_file);
            },
            .directory => {
                destination.makeDirZ(p.path) catch |err| switch (err) {
                    error.PathAlreadyExists => {
                        // Should always continue in this case
                        continue;
                    },
                    else => {
                        // This suggests something has gone rather wrong, but can still attempt to link files/other directories
                        try stderr.print(overwrite_prev_line ++ "Error creating directory\n\n", .{});
                        continue;
                    },
                };
            },
            else => {
                try stderr.print(overwrite_prev_line ++ "skipping non-file: {s}, {any}\n\n", .{ p.path, p.kind });
            },
        }
    }
    try stderr.print(overwrite_prev_line ++ "Linked  {d} files\n", .{file_count});
    try bw.flush();
}
