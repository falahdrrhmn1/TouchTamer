@echo off
rem Builds bin\TouchTamer.exe. Needs a 64-bit MinGW-w64 g++ and windres on PATH, for example w64devkit.
setlocal
cd /d "%~dp0src" || exit /b 1
if not exist ..\build mkdir ..\build

windres touchtamer.rc -O coff -o ..\build\touchtamer_res.o || exit /b 1

set FLAGS=-std=c++17 -Os -s -municode -mwindows -static -fno-exceptions -fno-rtti
set LIBS=-lgdiplus -lwbemuuid -lole32 -loleaut32 -luuid -lhid -ldwmapi

g++ %FLAGS% -o ..\bin\TouchTamer.exe touchtamer.cpp ..\build\touchtamer_res.o %LIBS% || exit /b 1
echo Built bin\TouchTamer.exe

if /i "%~1"=="test" (
    g++ %FLAGS% -DTOUCHTAMER_TEST -o ..\build\TouchTamer_test.exe touchtamer.cpp ..\build\touchtamer_res.o %LIBS% || exit /b 1
    g++ -std=c++17 -Wall -Wextra -O1 -I. -o ..\build\test_gesture.exe ..\tests\test_gesture.cpp || exit /b 1
    g++ -Os -static -mwindows -o ..\build\testwin.exe ..\tests\testwin.cpp || exit /b 1
    echo Built the test programs in build\
)
