@echo off
rem Build script for bfxr - all dependencies handled by CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build --config Release
if errorlevel 1 exit /b 1
echo.
echo SUCCESS
echo   CLI: build\bfxr.exe
echo   GUI: build\bfxr_gui.exe
