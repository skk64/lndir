const std = @import("std");
const builtin = @import("builtin");
// const Dir = std.Io.Dir;
const Allocator = std.mem.Allocator;
const Io = std.Io;
const assert = std.debug.assert;
const linux = std.os.linux;

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

const Args = struct {
    source_path: ?[*:0]const u8 = null,
    destination_path: ?[*:0]const u8 = null,
    help: bool = false,

    fn parse(argv: []const [*:0]const u8) Args {
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

pub fn main(init: std.process.Init) !void {
    const argv = init.minimal.args.vector;
    const args = Args.parse(argv);
    const io = init.io;

    // const cwd = std.Io.Dir.cwd();
    var stdout_file = std.Io.File.stdout();
    var stdout_buf: [4096]u8 = undefined;
    var stdout_writer = stdout_file.writer(io, &stdout_buf);
    const stdout = &stdout_writer.interface;

    var stderr_file = std.Io.File.stderr();
    var stderr_buf: [4096]u8 = undefined;
    var stderr_writer = stderr_file.writer(io, &stderr_buf);
    const stderr = &stderr_writer.interface;

    defer stdout.flush() catch {};
    defer stderr.flush() catch {};

    if (args.help or args.source_path == null or args.destination_path == null) {
        try stdout.print(help_message, .{argv[0]});
        std.process.exit(1);
    }

    ln_all_files_in_dir(io, stdout, stderr, args.source_path.?, args.destination_path.?) catch |err| {
        try stderr.print("{any}\n\n", .{err});
        try stdout.print(help_message, .{argv[0]});
        std.process.exit(1);
    };
}

fn ln_all_files_in_dir(io: Io, stdout: *Io.Writer, stderr: *Io.Writer, source_path: [*:0]const u8, destination_path: [*:0]const u8) !void {
    _ = &stdout;
    const destination_path_span: []const u8 = std.mem.span(destination_path);
    const source_path_span: []const u8 = std.mem.span(source_path);

    const cwd = std.Io.Dir.cwd();

    const destination = try std.Io.Dir.createDirPathOpen(cwd, io, destination_path_span, .{
        .open_options = .{
            .iterate = true,
            .follow_symlinks = false,
            .access_sub_paths = true,
        },
    });
    const source = try std.Io.Dir.createDirPathOpen(cwd, io, source_path_span, .{
        .open_options = .{
            .iterate = true,
            .follow_symlinks = false,
            .access_sub_paths = true,
        },
    });

    var file_buf: [std.posix.PATH_MAX]u8 = undefined;
    var source_path_buf: [std.posix.PATH_MAX]u8 = undefined;

    const source_path_absolute_len = try source.realPath(io, &source_path_buf);
    const source_path_absolute = source_path_buf[0..source_path_absolute_len];

    // Need memory for walker and file path buffers
    // Walker uses bounded amount of memory, defined by maximum path lengths
    // so FixedBufferAllocator is suitable
    // Maximum memory requirement determined by getdents64() syscall (max of u16)
    // plus 2 paths (and some extra for allocator overhead)
    const mem_size = std.posix.PATH_MAX * 2 + std.math.maxInt(u16) + 0x1000;
    const array_size = switch (builtin.link_libc) {
        true => 0,
        false => mem_size,
    };
    var array_buf: [array_size]u8 = undefined;
    const mem_buffer: []u8 = switch (builtin.link_libc) {
        true => try std.heap.c_allocator.alloc(u8, mem_size),
        false => &array_buf,
    };
    defer if (builtin.link_libc) std.heap.c_allocator.free(mem_buffer);

    var fba = std.heap.FixedBufferAllocator.init(mem_buffer);
    const allocator = fba.allocator();

    var file_count: u64 = 0;

    // Ansi control codes for progress tracking on same line
    const csi: *const [2:0]u8 = &.{ std.ascii.control_code.esc, '[' };
    const move_cursor_prev_line = csi ++ "F";
    const erase_line = csi ++ "K";
    const overwrite_prev_line = move_cursor_prev_line ++ erase_line;

    // newline to compensate for first cursor_prev_line code
    try stderr.print("\n", .{});

    var walker = source.walk(allocator) catch unreachable;
    defer walker.deinit();
    while (walker.next(io) catch |e| blk: {
        try stderr.print("Error accessing files: {any}\n\n", .{e});
        break :blk null;
    }) |p| {

        // skip anything that would cause a recursive loop. (that would cause a file to be linked more than once)
        const file_path_absolute_len = try destination.realPath(io, &file_buf);
        var file_path_absolute = file_buf[0 .. file_path_absolute_len + 1];
        file_buf[file_path_absolute.len - 1] = '/';
        @memcpy(file_buf[file_path_absolute.len..][0..p.path.len], p.path);
        file_path_absolute.len += p.path.len + 1;
        if (std.mem.startsWith(u8, file_path_absolute, source_path_absolute)) {
            try stderr.print(overwrite_prev_line ++ "Cannot create link inside source directory. Skipping {s}\n\n", .{p.path});
            continue;
        }
        switch (p.kind) {
            .file, .sym_link => {
                // Infallible as memory is allocated already
                const source_file = std.fs.path.joinZ(allocator, &.{ source_path_span, p.path }) catch unreachable;
                const destination_file = std.fs.path.joinZ(allocator, &.{ destination_path_span, p.path }) catch unreachable;

                const link_result = std.os.linux.errno(std.os.linux.link(source_file, destination_file));
                if (link_result == .SUCCESS) {
                    file_count += 1;
                    try stderr.print(overwrite_prev_line ++ "Linking  {d} files\n", .{file_count});
                } else {
                    try stderr.print(overwrite_prev_line ++ "Error creating link '{s}': {any}\n\n", .{ destination_file, link_result });
                }
                allocator.free(destination_file);
                allocator.free(source_file);
            },
            .directory => {
                destination.createDir(io, p.path, .default_dir) catch |err| switch (err) {
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
}
