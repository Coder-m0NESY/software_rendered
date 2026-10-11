#include "texture.hpp"

#include <algorithm>   // std::clamp
#include <cmath>       // std::floor

Texture::Texture(int width, int height, const Color& fill)
    : width_(width > 0 ? width : 1),
      height_(height > 0 ? height : 1),
      pixels_(static_cast<std::size_t>(width_) *
                  static_cast<std::size_t>(height_),
              fill)
{
}

int Texture::wrap_index(int v, int n)
{
    if (n <= 0) return 0;
    return ((v % n) + n) % n;
}

Color Texture::texel(int x, int y) const
{
    if (pixels_.empty()) return Color();
    return pixels_[static_cast<std::size_t>(wrap_index(y, height_)) *
                       static_cast<std::size_t>(width_) +
                   static_cast<std::size_t>(wrap_index(x, width_))];
}

void Texture::set_texel(int x, int y, const Color& c)
{
    if (pixels_.empty()) return;
    pixels_[static_cast<std::size_t>(wrap_index(y, height_)) *
                static_cast<std::size_t>(width_) +
            static_cast<std::size_t>(wrap_index(x, width_))] = c;
}

Color Texture::sample_nearest(float u, float v) const
{
    // u、v 乘上边长得到「浮点像素坐标」，向下取整就是像素下标。
    //
    // 这里不把 u 夹在 [0,1]：uv 可以超出 1，那是「重复平铺」（tiling）——
    // 地板那种大平面就是靠它把一张小图铺满的。texel() 自带 wrap，
    // 负数（比如 u = -1.5）也会正确绕回另一侧。
    const int x = static_cast<int>(std::floor(u * static_cast<float>(width_)));
    const int y = static_cast<int>(std::floor(v * static_cast<float>(height_)));
    return texel(x, y);
}

Color Texture::sample_bilinear(float u, float v) const
{
    // 减半格：让 u = 0 落在「第一个像素的中心」，而不是它的左上角。
    // 不减的话整张贴图会往右下偏半格，放大后能看出来。
    const float fx = u * static_cast<float>(width_)  - 0.5f;
    const float fy = v * static_cast<float>(height_) - 0.5f;

    const float fx_floor = std::floor(fx);
    const float fy_floor = std::floor(fy);
    const int   x0 = static_cast<int>(fx_floor);
    const int   y0 = static_cast<int>(fy_floor);
    const float tx = fx - fx_floor;   // 0~1：离左边那个像素的中心多远
    const float ty = fy - fy_floor;

    // 周围的 4 个像素。x0 可能是 -1，texel() 会自动绕回来，所以不用特判
    const Color c00 = texel(x0,     y0);
    const Color c10 = texel(x0 + 1, y0);
    const Color c01 = texel(x0,     y0 + 1);
    const Color c11 = texel(x0 + 1, y0 + 1);

    // 先在 x 方向混出上下两行，再把这两行在 y 方向混起来 —— 两次线性，所以叫「双线性」
    auto mix1 = [](float t, uint8_t a, uint8_t b) -> float {
        return static_cast<float>(a) +
               (static_cast<float>(b) - static_cast<float>(a)) * t;
    };
    auto blend = [&](uint8_t a00, uint8_t a10, uint8_t a01, uint8_t a11) -> uint8_t {
        const float top    = mix1(tx, a00, a10);
        const float bottom = mix1(tx, a01, a11);
        // +0.5 是四舍五入：浮点截断会让整张图系统性地偏暗一点点
        return static_cast<uint8_t>(
            std::clamp(top + (bottom - top) * ty + 0.5f, 0.0f, 255.0f));
    };

    return Color(blend(c00.r, c10.r, c01.r, c11.r),
                 blend(c00.g, c10.g, c01.g, c11.g),
                 blend(c00.b, c10.b, c01.b, c11.b),
                 255);
}

Texture Texture::make_checkerboard(int size, int cell,
                                   const Color& a, const Color& b)
{
    Texture t(size, size, a);
    if (cell <= 0) return t;

    for (int y = 0; y < t.height(); ++y) {
        for (int x = 0; x < t.width(); ++x) {
            // 用整除而不是取模：格子边长是 cell，x / cell 就是第几格。
            // 行号列号之和为奇数 → 涂另一种颜色，交错就出来了
            const bool odd = ((x / cell) + (y / cell)) % 2 != 0;
            t.set_texel(x, y, odd ? b : a);
        }
    }
    return t;
}