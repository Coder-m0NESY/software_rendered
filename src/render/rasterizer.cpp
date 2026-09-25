#include "rasterizer.hpp"

#include "core/color.hpp"
#include "core/pixelbuffer.hpp"

#include <cstdlib>   // std::abs
#include <algorithm>  // std::min, std::max

// ============================================================
//  draw_line：Bresenham 直线
// ============================================================

void draw_line(PixelBuffer& fb,
               int x0, int y0, int x1, int y1,
               uint32_t color)
{
    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;

    if (dx >= dy) {
        int D = 2 * dy - dx;
        for (int i = 0; i <= dx; ++i) {
            fb.put_pixel(x0, y0, color);
            if (D > 0) {
                y0 += sy;
                D = D + 2 * dy - 2 * dx;
            } else {
                D = D + 2 * dy;
            }
            x0 += sx;
        }
    } else {
        int D = 2 * dx - dy;
        for (int i = 0; i <= dy; ++i) {
            fb.put_pixel(x0, y0, color);
            if (D > 0) {
                x0 += sx;
                D = D + 2 * dx - 2 * dy;
            } else {
                D = D + 2 * dx;
            }
            y0 += sy;
        }
    }
}

void draw_line(PixelBuffer& fb,
               int x0, int y0, int x1, int y1,
               const Color& color)
{
    draw_line(fb, x0, y0, x1, y1, color.to_pixel());
}

// ============================================================
//  draw_triangle_edge：边函数三角形填充
// ============================================================
//
// 核心思路：
//   ① 用三条边（首尾相接绕一圈）各算一个边函数：
//        w0 = edge_cross(V0, V1, P)   边 V0→V1
//        w1 = edge_cross(V1, V2, P)   边 V1→V2
//        w2 = edge_cross(V2, V0, P)   边 V2→V0
//   ② 三个 w 同号 → 在三角形内
//
// 和重心坐标法的关系：
//   w0 + w1 + w2 = 2 × 三角形面积（恒定值）。把这和当成分母一除，
//   得到的正好就是三个重心坐标 —— 换句话说：
//     「重心坐标法」= 「边函数法」+ 一次除法。
//
// 权重对应关系（很容易搞反，注意）：
//   w0 是边 V0→V1 的边函数，它对应的是"对面那个顶点 V2" → 权重给 V2
//   w1 是边 V1→V2 的边函数 → 权重给 V0
//   w2 是边 V2→V0 的边函数 → 权重给 V1
//
// 性能提示：w 是 x、y 的线性函数，所以可以增量递推：
//   Δw/Δx 和 Δw/Δy 都是常量，内层循环只做加法即可，无需重复乘法。
//   这里为保持和教程一致，先用最直白的"每像素重算"版本。

namespace {

// 边函数：点 P 在边 (V0→V1) 的哪一侧。
// 返回值 = 2 × 子三角形 V0-V1-P 的有向面积。
// 正/负代表两侧，0 代表在边上。
int edge_cross(int x0, int y0, int x1, int y1, int px, int py)
{
    return (px - x0) * (y1 - y0) - (py - y0) * (x1 - x0);
}

// 浮点 RGB，用于平滑着色的插值
struct RGBf {
    float r, g, b;
};

// 把一个 uint32_t 像素拆成 RGBf
RGBf unpack(uint32_t pixel, const SDL_PixelFormat* fmt)
{
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixel, fmt, &r, &g, &b, &a);
    return { static_cast<float>(r), static_cast<float>(g), static_cast<float>(b) };
}

// 拆三个
void unpack3(uint32_t c0, uint32_t c1, uint32_t c2,
             const SDL_PixelFormat* fmt,
             RGBf& out0, RGBf& out1, RGBf& out2)
{
    out0 = unpack(c0, fmt);
    out1 = unpack(c1, fmt);
    out2 = unpack(c2, fmt);
}

// 按三个权重混合三个颜色
RGBf mix_rgb(float w0, const RGBf& c0,
             float w1, const RGBf& c1,
             float w2, const RGBf& c2)
{
    return {
        w0 * c0.r + w1 * c1.r + w2 * c2.r,
        w0 * c0.g + w1 * c1.g + w2 * c2.g,
        w0 * c0.b + w1 * c1.b + w2 * c2.b,
    };
}

// 把 RGBf 打包回 uint32_t 像素
uint32_t pack_rgb(const RGBf& c, const SDL_PixelFormat* fmt)
{
    return SDL_MapRGBA(fmt,
        static_cast<Uint8>(c.r),
        static_cast<Uint8>(c.g),
        static_cast<Uint8>(c.b),
        255);
}

}  // namespace

// ---- 平面着色版 ----
void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0,
                        int x1, int y1,
                        int x2, int y2,
                        uint32_t color)
{
    // ---- 包围盒（顺带裁剪到画布内）----
    const int min_x = std::max(0, std::min({x0, x1, x2}));
    const int min_y = std::max(0, std::min({y0, y1, y2}));
    const int max_x = std::min(fb.width()  - 1, std::max({x0, x1, x2}));
    const int max_y = std::min(fb.height() - 1, std::max({y0, y1, y2}));

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {

            // 三条边各算一次边函数（= 2 × 对应子三角形的有向面积）
            const int w0 = edge_cross(x0, y0, x1, y1, x, y);  // 边 V0→V1
            const int w1 = edge_cross(x1, y1, x2, y2, x, y);  // 边 V1→V2
            const int w2 = edge_cross(x2, y2, x0, y0, x, y);  // 边 V2→V0

            // 三个同号 → 在三角形内（不假设绕序，正负都接受）
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                fb.put_pixel(x, y, color);
            }
        }
    }
}

// ---- 平滑着色版 ----
void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0,
                        int x1, int y1,
                        int x2, int y2,
                        uint32_t c0, uint32_t c1, uint32_t c2)
{
    const int min_x = std::max(0, std::min({x0, x1, x2}));
    const int min_y = std::max(0, std::min({y0, y1, y2}));
    const int max_x = std::min(fb.width()  - 1, std::max({x0, x1, x2}));
    const int max_y = std::min(fb.height() - 1, std::max({y0, y1, y2}));

    const SDL_PixelFormat* fmt = fb.format();

    RGBf C0, C1, C2;
    unpack3(c0, c1, c2, fmt, C0, C1, C2);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {

            const int w0 = edge_cross(x0, y0, x1, y1, x, y);
            const int w1 = edge_cross(x1, y1, x2, y2, x, y);
            const int w2 = edge_cross(x2, y2, x0, y0, x, y);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (w0 <= 0 && w1 <= 0 && w2 <= 0)) {

                // w0+w1+w2 = 2 × 三角形面积，是个恒定值。
                // 除以它，w 就变成了重心坐标（三个权重，和为 1）。
                const int sum = w0 + w1 + w2;
                if (sum == 0) continue;     // 退化三角形，跳过

                const float inv = 1.0f / static_cast<float>(sum);
                const float a0 = w1 * inv;  // V0 的权重
                const float a1 = w2 * inv;  // V1 的权重
                const float a2 = w0 * inv;  // V2 的权重

                fb.put_pixel(x, y, pack_rgb(mix_rgb(a0, C0, a1, C1, a2, C2), fmt));
            }
        }
    }
}

// ---- Color 版 ----
void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0, int x1, int y1, int x2, int y2,
                        const Color& color)
{
    draw_triangle_edge(fb, x0, y0, x1, y1, x2, y2, color.to_pixel());
}

void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0, int x1, int y1, int x2, int y2,
                        const Color& c0, const Color& c1, const Color& c2)
{
    draw_triangle_edge(fb, x0, y0, x1, y1, x2, y2,
                       c0.to_pixel(), c1.to_pixel(), c2.to_pixel());
}

// ---- 带深度测试版 ----
void draw_triangle_depth(PixelBuffer& fb,
                         int x0, int y0, float z0,
                         int x1, int y1, float z1,
                         int x2, int y2, float z2,
                         uint32_t color)
{
    const int min_x = std::max(0, std::min({x0, x1, x2}));
    const int min_y = std::max(0, std::min({y0, y1, y2}));
    const int max_x = std::min(fb.width()  - 1, std::max({x0, x1, x2}));
    const int max_y = std::min(fb.height() - 1, std::max({y0, y1, y2}));

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {

            const int w0 = edge_cross(x0, y0, x1, y1, x, y);
            const int w1 = edge_cross(x1, y1, x2, y2, x, y);
            const int w2 = edge_cross(x2, y2, x0, y0, x, y);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (w0 <= 0 && w1 <= 0 && w2 <= 0)) {

                // 重心坐标插值算深度
                const int sum = w0 + w1 + w2;
                if (sum == 0) continue;

                const float inv = 1.0f / static_cast<float>(sum);
                const float z = (w1 * z0 + w2 * z1 + w0 * z2) * inv;

                // 深度测试：更近才覆盖
                if (z < fb.depth_at(x, y)) {
                    fb.put_pixel(x, y, color);
                    fb.set_depth(x, y, z);
                }
            }
        }
    }
}

// ============================================================
//  通用入口：三种算法里默认用边函数法（最快、最通用）
// ============================================================

void draw_triangle(PixelBuffer& fb,
                   int x0, int y0, int x1, int y1, int x2, int y2,
                   uint32_t color)
{
    draw_triangle_edge(fb, x0, y0, x1, y1, x2, y2, color);
}

void draw_triangle(PixelBuffer& fb,
                   int x0, int y0, int x1, int y1, int x2, int y2,
                   const Color& color)
{
    draw_triangle_edge(fb, x0, y0, x1, y1, x2, y2, color);
}

void draw_triangle(PixelBuffer& fb,
                   int x0, int y0, int x1, int y1, int x2, int y2,
                   uint32_t c0, uint32_t c1, uint32_t c2)
{
    draw_triangle_edge(fb, x0, y0, x1, y1, x2, y2, c0, c1, c2);
}

void draw_triangle(PixelBuffer& fb,
                   int x0, int y0, int x1, int y1, int x2, int y2,
                   const Color& c0, const Color& c1, const Color& c2)
{
    draw_triangle_edge(fb, x0, y0, x1, y1, x2, y2, c0, c1, c2);
}
