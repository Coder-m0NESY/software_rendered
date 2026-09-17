@echo off
rem 本脚本是 UTF-8 编码，切换代码页避免中文变乱码
chcp 65001 >nul
setlocal

rem ============================================================
rem  SoftRenderer 一键编译脚本（Windows / MinGW-w64 + CMake）
rem  用法：双击，或在 cmd / PowerShell 里执行  .\build.bat
rem        想保留控制台输出：.\build.bat console
rem ============================================================

rem --- 1. 本机 SDL2 的安装位置（只改这一行） -------------------
set "SDL2_ROOT=D:\SDL2\SDL2-devel-2.32.4-mingw\SDL2-2.32.4\x86_64-w64-mingw32"

set "ROOT=%~dp0"
set "BUILD_DIR=%ROOT%build"

rem CMake 更认正斜杠
set "SDL2_ROOT=%SDL2_ROOT:\=/%"

rem --- 2. 检查工具链 -------------------------------------------
where g++ >nul 2>nul
if errorlevel 1 (
    echo [错误] PATH 里找不到 g++，请把 MSYS2 的 ucrt64\bin 加入 PATH
    exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [错误] PATH 里找不到 cmake
    exit /b 1
)

where mingw32-make >nul 2>nul
if errorlevel 1 (
    echo [错误] PATH 里找不到 mingw32-make
    exit /b 1
)

rem --- 3. 检查 SDL2 --------------------------------------------
if not exist "%SDL2_ROOT%/include/SDL2/SDL.h" (
    echo [错误] 在 %SDL2_ROOT% 下找不到 include/SDL2/SDL.h
    echo         请检查 build.bat 里的 SDL2_ROOT
    exit /b 1
)

rem --- 4. 配置 + 编译 ------------------------------------------
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%"

set "SDL_CONSOLE=OFF"
if /i "%~1"=="console" set "SDL_CONSOLE=ON"

cmake -G "MinGW Makefiles" ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DSDL2_ROOT="%SDL2_ROOT%" ^
      -DSDL_CONSOLE=%SDL_CONSOLE% ..
if errorlevel 1 ( popd & exit /b 1 )

mingw32-make -j
if errorlevel 1 ( popd & exit /b 1 )

popd
echo.
echo [完成] 可执行文件：%BUILD_DIR%\SoftRenderer.exe
