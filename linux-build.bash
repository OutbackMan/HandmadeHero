# sh is a POSIX specification for a scripting language. sh will be symlinked to an implementation (this may have more features)
#!/bin/bash

# header, segment, section (runtime)
# wc --bytes, objdump -M intel -d; readelf --symbols, readelf --string-dump .comment <output>
# machine word is width of register, however word in assembly is 16 bits
# rbp params, rsp local
# --> vdso.so allows for certain syscalls to be executed in user space
# --> libm.so, librt.so are filter libraries to libc (legacy)

# NOTE(Ryan): On linux, libc doesn't just provide crt functions it also wraps syscalls pertaining to POSIX. Without it,
# we would have to write syscalls manually.
# On windows, is just crt so can do without it

# download llvm binary adding to /etc/profile
common_compiler_flags="-Wall -Wextra -Wpedantic -Wfloat-equal -Wunreachable-code -Wshadow -fuse-ld=lld -lX11 -lasound -ludev"

# libx11-dev libasound2-dev libudev-dev
# ensure llvm-symbolizer on $PATH 
debug_compiler_flags="-g -fno-omit-frame-pointer -fsanitize=address -fno-optimize-sibling-calls"

release_compiler_flags="-Ofast"

[ ! -d "build" ] && mkdir build
pushd build

clang $common_compiler_flags $debug_compiler_flags ../code/linux-hh.c -o linux-hh

popd
