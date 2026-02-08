/// To run the tests, run:
/// `zig build test`
///
/// To automatically build as source files are edited:
/// `zig build --watch --summary all`
///
const std = @import("std");
const zon = @import("build.zig.zon");

const c_source_files = [_][:0]const u8{
    "src/lndir.c",
    "src/string_list.c",
};
const c_main_file = "src/main.c";

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // c exe
    const exe_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });
    const WerrorIfDebug = if (optimize == .Debug) "-Werror" else "";
    exe_mod.addCSourceFiles(.{
        .files = &c_source_files,
        .flags = &.{WerrorIfDebug},
    });
    exe_mod.addCSourceFile(.{ .file = b.path(c_main_file) });
    exe_mod.link_libc = true;
    exe_mod.linkSystemLibrary("uring", .{ .preferred_link_mode = .dynamic });
    exe_mod.addCMacro("VERSION", "\"" ++ zon.version ++ " (z)\"");

    const c_exe = b.addExecutable(.{
        .name = "lndir",
        .root_module = exe_mod,
    });
    b.installArtifact(c_exe);

    // run steps
    const run_cmd = b.addRunArtifact(c_exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    // test steps
    const test_mod = b.createModule(.{
        .root_source_file = b.path("src/tests.zig"),
        .target = target,
        .optimize = optimize,
    });
    test_mod.addCSourceFiles(.{ .files = &c_source_files });
    test_mod.link_libc = true;
    test_mod.linkSystemLibrary("uring", .{ .preferred_link_mode = .dynamic });

    const string_mod = translate_c_file(b, optimize, target, "src/string_list.h");
    const lndir_mod = translate_c_file(b, optimize, target, "src/lndir.h");
    test_mod.addImport("string_list", string_mod);
    test_mod.addImport("lndir", lndir_mod);

    const exe_unit_tests = b.addTest(.{ .root_module = test_mod });
    const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_exe_unit_tests.step);
}

fn translate_c_file(
    b: *std.Build,
    opt: std.builtin.OptimizeMode,
    target: std.Build.ResolvedTarget,
    path: []const u8,
) *std.Build.Module {
    const translate_c = b.addTranslateC(.{
        .root_source_file = b.path(path),
        .optimize = opt,
        .target = target,
    });
    translate_c.addIncludePath(b.path("src/"));
    const translate_c_mod = translate_c.createModule();
    translate_c.link_libc = true;
    translate_c_mod.link_libc = true;
    return translate_c_mod;
}
