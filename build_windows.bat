@echo off
rem ============================================================
rem  AKASH-DARPAN — build_windows.bat
rem  Requires: Visual Studio 2022, CMake, vcpkg
rem ============================================================
echo ╔══════════════════════════════════════════════════════╗
echo ║  AKASH-DARPAN Windows Build Script                  ║
echo ╚══════════════════════════════════════════════════════╝

where cmake >nul 2>nul || (echo CMake not found. Install from https://cmake.org && exit /b 1)

rem ── Install deps via vcpkg (if available) ─────────────
where vcpkg >nul 2>nul
if %ERRORLEVEL%==0 (
    echo [Setup] Installing via vcpkg...
    vcpkg install glfw3:x64-windows opengl:x64-windows
)

rem ── Configure ─────────────────────────────────────────
if not exist build mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" 2>nul || ^
cmake .. -G "Visual Studio 17 2022" -A x64

rem ── Build ─────────────────────────────────────────────
cmake --build . --config Release --parallel
cd ..

echo.
echo ╔══════════════════════════════════════════════════════╗
echo ║  Done!  Run: build\Release\akash_darpan.exe          ║
echo ╚══════════════════════════════════════════════════════╝
