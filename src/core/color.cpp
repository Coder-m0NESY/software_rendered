#include "color.hpp"

// 0xAARRGGBB 和 Color 之间的转换，手写位移，不依赖 SDL。
//
// 这个位序就是 window.cpp 里那块 SDL_PIXELFORMAT_ARGB8888 纹理的位序，
// 所以 CPU 缓冲区可以直接往纹理里怼，中间不用再转一道。

Color Color::from_pixel(uint32_t pixel)
{
    return Color(static_cast<uint8_t>((pixel >> 16) & 0xFFu),
                 static_cast<uint8_t>((pixel >>  8) & 0xFFu),
                 static_cast<uint8_t>( pixel        & 0xFFu),
                 static_cast<uint8_t>((pixel >> 24) & 0xFFu));
}

uint32_t Color::to_pixel() const
{
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) <<  8) |
            static_cast<uint32_t>(b);
}
