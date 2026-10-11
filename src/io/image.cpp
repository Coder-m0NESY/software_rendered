#include "image.hpp"

#include <SDL.h>

#include <cstdint>
#include <cstdio>

namespace io {

bool load_texture_bmp(const std::string& path, Texture& out)
{
    SDL_Surface* raw = SDL_LoadBMP(path.c_str());
    if (raw == nullptr) {
        std::printf("读取贴图失败 %s：%s\n", path.c_str(), SDL_GetError());
        return false;
    }

    // 统一转成 ARGB8888：这样每个像素固定 4 字节，取颜色时不用管原始格式
    // （BMP 可能是 8 位调色板、16 位 565、24 位 BGR……各不一样）
    SDL_Surface* surface = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(raw);
    if (surface == nullptr) {
        std::printf("转换贴图格式失败 %s：%s\n", path.c_str(), SDL_GetError());
        return false;
    }

    Texture tex(surface->w, surface->h);

    SDL_LockSurface(surface);
    const auto* pixels = static_cast<const std::uint32_t*>(surface->pixels);
    // pitch 是一行占的字节数，可能比 width*4 大（每行末尾有对齐填充），
    // 所以按「一行几个 word」来算下标，不能直接用 y * width + x
    const int pitch_words = surface->pitch / 4;

    for (int y = 0; y < surface->h; ++y) {
        for (int x = 0; x < surface->w; ++x) {
            const std::uint32_t p = pixels[y * pitch_words + x];
            tex.set_texel(x, y,
                          Color(static_cast<std::uint8_t>((p >> 16) & 0xFF),
                                static_cast<std::uint8_t>((p >> 8) & 0xFF),
                                static_cast<std::uint8_t>(p & 0xFF)));
        }
    }

    SDL_UnlockSurface(surface);
    SDL_FreeSurface(surface);

    out = tex;
    return true;
}

}   // namespace io