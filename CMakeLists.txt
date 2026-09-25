cmake_minimum_required(VERSION 3.16)
project(SoftRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "构建类型" FORCE)
endif()

# ---------------------------------------------------------------------- SDL2
# 本机解压出来的 SDL2（MinGW x86_64 版）根目录。
# 换机器只改这一行，或者在配置时覆盖：cmake -DSDL2_ROOT="新的/路径" ..
set(SDL2_ROOT "D:/SDL2/SDL2-devel-2.32.4-mingw/SDL2-2.32.4/x86_64-w64-mingw32"
    CACHE PATH "SDL2 的 x86_64-w64-mingw32 目录")

# 提示：下面给用户看的 message / COMMENT 一律用英文。
# cmake 在 Windows 控制台会按 ANSI 代码页输出，中文容易变成乱码。
if(NOT EXISTS "${SDL2_ROOT}/include/SDL2/SDL.h")
    message(FATAL_ERROR
        "SDL2 not found: ${SDL2_ROOT}/include/SDL2/SDL.h does not exist.\n"
        "SDL2_ROOT must point to the unzipped x86_64-w64-mingw32 directory.")
endif()

set(SDL2_INCLUDE_DIR "${SDL2_ROOT}/include/SDL2")   # 注意：是 include/SDL2，不是 include
set(SDL2_LIB_DIR     "${SDL2_ROOT}/lib")
set(SDL2_DLL         "${SDL2_ROOT}/bin/SDL2.dll")

# 开发期想用 printf 打日志就加上 -DSDL_CONSOLE=ON（会保留控制台窗口）
option(SDL_CONSOLE "保留控制台窗口，方便看 printf 输出" OFF)

# -------------------------------------------------------------------- target
add_executable(SoftRenderer
    src/main.cpp
    src/core/color.cpp
    src/core/window.cpp
    src/core/pixelbuffer.cpp
    src/math/vec.cpp
    src/math/mat.cpp
    src/render/rasterizer.cpp
    src/render/renderer.cpp
)

# src 也加进搜索路径，这样任何文件里都能写 #include "core/color.hpp"，
# 不用管自己在哪一层目录
target_include_directories(SoftRenderer PRIVATE
    "${SDL2_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_directories(SoftRenderer PRIVATE "${SDL2_LIB_DIR}")

# 链接顺序不能改：mingw32 -> SDL2main -> SDL2
#   mingw32  : 提供 WinMain 的调用方（crtexewin.o）
#   SDL2main : 提供 WinMain，它再去调用被宏改名后的 SDL_main
#   SDL2     : 真正的库
target_link_libraries(SoftRenderer PRIVATE mingw32 SDL2main SDL2)

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT SDL_CONSOLE)
    target_link_options(SoftRenderer PRIVATE -mwindows)
endif()

# SDL2.dll 不在系统目录里，编译完直接拷到 exe 旁边，双击就能跑
add_custom_command(TARGET SoftRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DLL}" "$<TARGET_FILE_DIR:SoftRenderer>"
    COMMENT "Copying SDL2.dll next to the executable")
