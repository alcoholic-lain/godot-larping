# SConstruct
import os

env = SConscript("godot-cpp/SConstruct")

if env["platform"] == "windows":
    env.Append(CPPFLAGS=["/std:c++17"])
else:
    env.Append(CPPFLAGS=["-std=c++17"])

sources = Glob("src/*.cpp")

library = env.SharedLibrary(
    "bin/libmnms_port{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)
