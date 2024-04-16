Import("env")

env.Replace(
    # include toolchain paths + output to compile_commands.json
    COMPILATIONDB_INCLUDE_TOOLCHAIN=True,
    COMPILATIONDB_PATH="compile_commands.json",
)
