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

    if (is_subpath(source_path_span, destination_path_span)) {
        try stderr.print("Cannot link into source directory\n", .{});
    }

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

    // Need memory for walker
    // Walker uses bounded amount of memory, defined by maximum path lengths
    // so FixedBufferAllocator is suitable
    // Maximum memory requirement determined by getdents64() syscall (max of u16)
    // and some extra for allocator overhead
    const mem_size = std.math.maxInt(u16) + 0x1000;
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
        // The directory depth shouldn't get this high. Probably an error if it does
        if (p.depth() > 128) return error.DirectoryDepthTooHigh;

        switch (p.kind) {
            .file, .sym_link => {
                std.Io.Dir.hardLink(source, p.path, destination, p.path, io, .{ .follow_symlinks = true }) catch |e| {
                    try stderr.print(overwrite_prev_line ++ "Error creating link '{s}': {any}\n\n", .{ p.path, e });
                    continue;
                };
                file_count += 1;
                try stderr.print(overwrite_prev_line ++ "Linking  {d} files\n", .{file_count});
            },
            .directory => {
                // destination.createDirPath(io, p.path);
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

///  **  /a/b/c, /a/b/d -> false
///  **  /a/b/c, /a/b/c/d -> true
///  **  /a/b/c, /a/b/c_d -> false
///  **  /a/b/c, /a//b/c/d -> true
fn is_subpath(root_path: []const u8, potential_subpath: []const u8) bool {
    const sep = std.fs.path.sep;
    if (std.mem.startsWith(u8, potential_subpath, root_path)) {
        if (potential_subpath.len == root_path.len) return true;
        assert(potential_subpath.len > root_path.len);
        if (potential_subpath[root_path.len] == sep) return true;
    }
    return false;
}
