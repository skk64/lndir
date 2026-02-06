const std = @import("std");

pub fn build(b: *std.Build) void {
    // options
    const staticBinary = b.option(bool, "static", "compile with musl to make a static executable") orelse false;
    const no_llvm = b.option(bool, "no-llvm", "Don't use LLVM backend for compilation") orelse false;
    // const target = b.standardTargetOptions(.{ .default_target = if (staticBinary) .{ .abi = .musl } else .{} });
    const target = b.standardTargetOptions(.{});
    var c_target = target;
    if (staticBinary) {
        c_target.result.abi = .musl;
        c_target.query.abi = .musl;
    }
    const optimize = b.standardOptimizeOption(.{});

    const exe_mod = b.createModule(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // zig exe
    const exe = b.addExecutable(.{
        .name = "lndir",
        .root_module = exe_mod,
    });
    if (no_llvm) {
        exe.use_llvm = false;
    }
    if (!staticBinary) {
        exe_mod.link_libc = true;
    }
    b.installArtifact(exe);

    // c exe
    const c_exe_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });
    const WerrorIfDebug = if (optimize == .Debug) "-Werror" else "";
    // c_exe_mod.addCSourceFile(.{ .file = b.path("src/lndir.c"), .flags = &.{WerrorIfDebug} });
    c_exe_mod.addCSourceFiles(.{
        .files = &.{
            "src/lndir_uring.c",
            "src/string_list.c",
            "src/main.c",
        },
        .flags = &.{WerrorIfDebug},
    });
    c_exe_mod.link_libc = true;
    c_exe_mod.linkSystemLibrary("uring", .{ .preferred_link_mode = .static });

    const c_exe = b.addExecutable(.{
        .name = "lndir_c",
        .root_module = c_exe_mod,
    });
    b.installArtifact(c_exe);

    // run steps
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    // test steps
    const exe_unit_tests = b.addTest(.{
        .root_module = exe_mod,
    });
    const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_exe_unit_tests.step);
}
