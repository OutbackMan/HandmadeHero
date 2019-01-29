#!/bin/bash

common_compiler_flags="-Wall -Wextra -Wpedantic -Wfloat-equal -Wunreachable-code -Wshadow -framework AppKit"

debug_compiler_flags="-g -fno-omit-frame-pointer -fno-optimize-sibling-calls"

release_compiler_flags="-Ofast"

[ ! -d "build" ] && mkdir build
pushd build

clang $common_compiler_flags $debug_compiler_flags ../code/mac-hh.c -o mac-hh

popd
