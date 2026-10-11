#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <string>

#include "core/texture.hpp"

// ============================================================================
// io/image ── 把一张图片文件读成 Texture
//
// 分工：
//   core/texture  只管「采样」—— 拿到一张已经在内存里的像素图，怎么取色
//   io/image      只管「读盘」—— 把文件里的像素搬到内存，装成 Texture
// 这样 core/texture 不需要认识任何文件格式，也不需要认识 SDL。
//
// 分层：io/ 依赖 core/（Texture、Color），并把 SDL 关在这一层里。
// 全项目认识 SDL 的只有三个地方：core/window、io/image、src/main.cpp。
//
// 为什么是 BMP：SDL2 自带 SDL_LoadBMP，不用额外依赖 SDL_image。
// PNG / JPG 要靠 SDL_image（另一个库），以后真需要再加一条加载路径。
// 本项目就是「先用 BMP 跑通读图这条路」，PNG 是同一套代码换一个读取函数的事。
// ============================================================================
namespace io {

// 读一张 BMP，成功时写入 out 并返回 true。
// 失败（文件不存在、格式不认识）时返回 false，out 不动，并往控制台打一行原因。
//
// 用「bool + 输出参数」而不是「返回 Texture」：Texture 的构造函数会把非法尺寸
// 夹成至少 1×1，所以返回一个 Texture 没法表示「读失败」这件事。
bool load_texture_bmp(const std::string& path, Texture& out);

}   // namespace io

#endif