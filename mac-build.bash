#!/bin/bash

# Download xcode command line tools and llvm binary adding to /etc/profile
# Use otool -L <file>
common_compiler_flags="-fuse-ld=lld -Wall -Wextra -Wpedantic -Wfloat-equal -Wunreachable-code -Wshadow -framework AppKit"

debug_compiler_flags="-g -fno-omit-frame-pointer -fno-optimize-sibling-calls"

release_compiler_flags="-Ofast"

[ ! -d "build" ] && mkdir build
pushd build

clang $common_compiler_flags $debug_compiler_flags ../code/mac-hh.m -o mac-hh

popd
