const std = @import("std");

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.Build) void {
    // options
    const staticBinary = b.option(bool, "static", "compile with musl to make a static executable (Linux only)") orelse false;
    const no_llvm = b.option(bool, "no-llvm", "Don't use LLVM backend for compilation") orelse false;
    const target = b.standardTargetOptions(.{ .default_target = if (staticBinary) .{ .abi = .musl } else .{} });
    const optimize = b.standardOptimizeOption(.{});

    // zig exe
    const exe = b.addExecutable(.{
        .name = "lndir",
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    if (no_llvm) {
        exe.use_llvm = false;
    }
    if (!staticBinary) {
        exe.linkLibC();
    }
    b.installArtifact(exe);

    // c exe
    const c_exe = b.addExecutable(.{
        .name = "lndir_c",
        .target = target,
        .optimize = optimize,
    });
    const WerrorIfDebug = if (optimize == .Debug) "-Werror" else "";
    c_exe.addCSourceFile(.{ .file = b.path("src/lndir.c"), .flags = &.{WerrorIfDebug} });
    c_exe.linkLibC();
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
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_exe_unit_tests.step);
}
