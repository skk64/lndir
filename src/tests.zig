const std = @import("std");
const Io = std.Io;
const testing = std.testing;
const sl = @import("string_list");
const lndir = @import("lndir");

extern fn hardlink_file_list_iouring(file_list: *sl.StringListIter, src_dir: [*:0]const u8, dest_dir: [*:0]const u8) c_int;

test "lndir link" {
    const io = testing.io;
    var list: sl.StringList = .{ .first = null, .last = null };
    var iter: sl.StringListIter = sl.StringList_iterate(&list);

    const source_dir = "foo";
    const destination_dir = "bar";
    const files = [_][:0]const u8{
        "a",
        "b",
        "foo",
        "bar",
        "stuff",
    };

    for (files) |f| sl.StringList_add_nullterm(&list, f);

    const cwd = std.Io.Dir.cwd();

    defer std.Io.Dir.deleteTree(cwd, io, source_dir) catch {};
    defer std.Io.Dir.deleteTree(cwd, io, destination_dir) catch {};

    try std.Io.Dir.deleteTree(cwd, io, source_dir);
    try std.Io.Dir.deleteTree(cwd, io, destination_dir);
    try std.Io.Dir.createDir(cwd, io, source_dir, .default_dir);
    try std.Io.Dir.createDir(cwd, io, destination_dir, .default_dir);
    inline for (files) |f| {
        const file = try std.Io.Dir.createFile(cwd, io, source_dir ++ "/" ++ f, .{});
        file.close(io);
    }
    const result = hardlink_file_list_iouring(@ptrCast(&iter), source_dir, destination_dir);
    try testing.expectEqual(0, result);

    inline for (files) |f| {
        try expect_file_exists(io, destination_dir ++ "/" ++ f);
    }
}

fn expect_file_exists(io: Io, filename: [:0]const u8) !void {
    const cwd = std.Io.Dir.cwd();
    const f = try std.Io.Dir.openFile(cwd, io, filename, .{ .mode = .read_only });
    f.close(io);
}

test "StringList consistency" {
    const strings = [_][:0]const u8{
        "/c",
        "various/tests/foo/z/x/asdf",
        "various/tests/foo/b",
        "various/tests/basic_in/b",
        "various/tests/basic_in/foo",
        "various/tests/foo_out/z/x/asdf",
        "various/tests/foo_out/z/c",
        "various/tests/foo_out/b",
        "various/tests/foo_out/a",
    } ** 60;

    var list: sl.StringList = .{ .first = null, .last = null };
    for (strings) |s| sl.StringList_add_nullterm(&list, s);

    var iter: sl.StringListIter = sl.StringList_iterate(&list);
    for (0..strings.len) |i| {
        const a_ptr = sl.StringListIter_next(&iter);
        const a = std.mem.span(a_ptr);
        const b = strings[i];
        try testing.expectEqualStrings(b, a);
    }
}
