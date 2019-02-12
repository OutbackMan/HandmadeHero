#!/bin/bash

common_compiler_flags="-Wall -Wextra -Wpedantic -Wfloat-equal -Wunreachable-code -Wshadow -fuse-ld=lld -lSDL2"

debug_compiler_flags="-g -fno-omit-frame-pointer -fsanitize=address -fno-optimize-sibling-calls"

release_compiler_flags="-Ofast"

[ ! -d "build" ] && mkdir build
pushd build

clang $common_compiler_flags $debug_compiler_flags ../code/hh.c -o hh

popd
