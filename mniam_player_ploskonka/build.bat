REM Dopasuj poniższe ścieżki:
set MINGW_PATH=D:\kuba_am\mingw64\bin
set CMAKE_PATH=D:\kuba_am\cmake-4.0.3-windows-x86_64\bin


set PATH=%PATH%;%MINGW_PATH%;%CMAKE_PATH%;

cmake -G "MinGW Makefiles" -B build
mingw32-make.exe -C build