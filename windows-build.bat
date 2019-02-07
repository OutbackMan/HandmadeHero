@ECHO OFF

REM NOTE(Ryan): Unable to use clang + lld + mingw due to bug documented here: https://bugs.llvm.org/show_bug.cgi?id=38939  
REM             Therefore, resort to using slower ld from mingw binutils for the time being

SET common_compiler_flags=-Wall -Wextra -Wpedantic -Wfloat-equal -Wunreachable-code -Wshadow^
 -target x86_64-windows-gnu -luser32 -lgdi32 -lxinput -ldsound

SET debug_compiler_flags=-fno-omit-frame-pointer -fno-optimize-sibling-calls -g

SET release_compiler_flags=-Ofast

IF NOT EXIST build MKDIR build
PUSHD build

clang.exe %common_compiler_flags% %debug_compiler_flags% ..\code\windows-hh.c -o windows-hh.exe

POPD
