@ECHO OFF

SET common_compiler_flags=-Wall -Wextra -Wpedantic -Wfloat-equal -Wunreachable-code -Wshadow^
 -fuse-ld=lld.exe -target x86_64-w64-mingw64 -lgdi32

SET debug_compiler_flags=-fno-omit-frame-pointer -fno-optimize-sibling-calls^
 -gcodeview -Wl,/debug,/pdb:windows-hh.pdb

SET release_compiler_flags=-Ofast

IF NOT EXIST build MKDIR build
PUSHD build
DEL hh.pdb > NUL 2> NUL

clang.exe %common_compiler_flags% %debug_compiler_flags% code\windows-hh.c -o windows-hh.exe

POPD
