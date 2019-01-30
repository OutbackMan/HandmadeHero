@ECHO OFF

REM NOTE(Ryan): As of January 2019, unable to use lld with clang/mingw. 
REM Therefore, resort to clang/msvc, i.e. visual studio c++ build tools and windows 10 SDK

SET common_compiler_flags=-Wall -Wextra -Wpedantic -Wfloat-equal -Wunreachable-code -Wshadow^
 -fuse-ld=lld-link.exe -luser32 -lgdi32 -lxinput

SET debug_compiler_flags=-fno-omit-frame-pointer -fno-optimize-sibling-calls^
 -g -gcodeview -Wl,/debug,/pdb:windows-hh.pdb

SET release_compiler_flags=-Ofast

IF NOT EXIST build MKDIR build
PUSHD build
DEL hh.pdb > NUL 2> NUL

clang.exe %common_compiler_flags% %debug_compiler_flags% ..\code\windows-hh.c -o windows-hh.exe

POPD
