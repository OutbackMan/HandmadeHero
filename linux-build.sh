# sh is a POSIX specification for a scripting language. sh will be symlinked to an implementation (this may have more features)
#!/bin/bash

common_compiler_flags="-Wall -Wextra -Wpedantic -Wfloat-equal -Wunreachable-code -Wshadow"

debug_compiler_flags="-g -fno-omit-frame-pointer -fno-optimize-sibling-calls"

release_compiler_flags="-Ofast"

[ ! -d "build" ] && mkdir build
pushd build

clang $common_compiler_flags $debug_compiler_flags ../code/linux-hh.c -o linux-hh

popd
