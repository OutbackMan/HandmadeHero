## Developer Environment
|                                | Windows (windows 10)       | Linux (ubuntu 18.04) | Mac (high sierra) |
| ------------------------------ | -------------------------- | -------------------- | ----------------- |
| BUILD REQUIREMENTS (ESSENTIAL) | [llvm][1]<br>[mingw-w64](https://sourceforge.net/projects/mingw-w64/) | [llvm][1]<br>`$ sudo apt-get install libx11-dev libasound2-dev libudev-dev` | [llvm][1]<br>`$ xcode-select --install` |
| DEBUGGING (OPTIONAL)           | [jre][2]<br>[cdt][3] | [jre][2]<br>[cdt][3]  | [jre][2]<br>[cdt][3] |
| CODING (OPTIONAL)              | [gvim](https://www.vim.org/download.php#pc) | `$ sudo apt-get install vim-gtk3`  | [macvim](https://github.com/macvim-dev/macvim/releases) |
| COMPILE AND RUN                | `> windows-build.bat`<br>`> build\windows-hh.exe`| `$ bash linux-build.bash`<br>`$ build/linux-hh` | `$ bash mac-build.bash`<br>`$ build/mac-hh` |

[1]: http://releases.llvm.org/
[2]: https://www.oracle.com/technetwork/java/javase/downloads/jre8-downloads-2133155.html
[3]: https://download.eclipse.org/tools/cdt/releases/9.6/cdt-9.6.0/rcp/
