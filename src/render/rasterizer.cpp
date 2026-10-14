#include "rasterizer.hpp"

#include "core/color.hpp"
#include "core/pixelbuffer.hpp"
#include "core/texture.hpp"
#include "render/blend.hpp"

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
//
// 为什么用 long long 而不是 int：
//   近平面裁剪会造出「贴在近平面边上」的顶点，它们除以很小的 w 之后，
//   屏幕坐标能到 ±2e5 量级。两个这样的差相乘就超过 int32 的 2.1e9，
//   符号会被溢出翻掉，于是三角形被随机误判成背面 / 覆盖判断整个错乱
//   （表现为地板缺一块、动一下又变）。long long 足够放下这个量级。
long long edge_cross(int x0, int y0, int x1, int y1, int px, int py)
{
    const long long dx0 = static_cast<long long>(px) - x0;
    const long long dy0 = static_cast<long long>(py) - y0;
    const long long dx1 = static_cast<long long>(x1) - x0;
    const long long dy1 = static_cast<long long>(y1) - y0;
    return dx0 * dy1 - dy0 * dx1;
}

// 背面剔除：屏幕空间有向面积判断这个三角形是不是背对着相机。
//
//   area2 = (x1-x0)*(y2-y0) - (x2-x0)*(y1-y0)
//
// 这算出来是「有向面积的两倍」，符号代表绕序。
//
// 判据（本项目采用的、也是屏幕空间里最省事的说法）：
//   屏幕坐标 x 向右、y 向下，**正面朝你的三角形，area2 > 0**。
// 等价说法：在物体外表面所在平面上、改用「x 向右 y 向上」的常规坐标系看，
// 正面三角形的顶点顺序是**顺时针（CW）**。
// （OpenGL 默认约定的正面是 CCW —— 那是「窗口 y 向上」的坐标系；本项目屏幕
//   y 向下，两者是同一个绕序在两种坐标轴朝向下换了符号，判定不受影响。
//   第 11 步实测过：把正面顶点代进去，area2 正好是正的。）
//
// 为什么背面就会变号：
//   面正对你  → 投影不改变绕序 → area2 > 0 → 正面，保留
//   面背对你  → 相当于从背面看，绕序翻转 → area2 < 0 → 背面，丢掉
//   退化（三点重合/共线）→ area2 == 0 → 画不出东西，一并丢掉
//
// 为什么值得做：立方体 12 个三角形里永远有 6 个是背对你的，
// 在填像素之前整块丢掉，等于白省一半的填充 —— 而且只有几行加法，
// 不需要任何额外状态。
//
// ⚠ 这一刀改掉了前几版的「不假设绕序」：从第 11 步起，
// 三角形填充**要求绕序正确**（正面 area2 > 0），否则模型会整体消失。
bool back_facing(int x0, int y0, int x1, int y1, int x2, int y2)
{
    // 同样用 long long：裁剪后的顶点屏幕坐标可能很大（见 edge_cross 的说明），
    // 用 int 算面积会在极端顶点上溢出、把正面判成背面。
    const long long area2 =
        (static_cast<long long>(x1) - x0) * (static_cast<long long>(y2) - y0) -
        (static_cast<long long>(x2) - x0) * (static_cast<long long>(y1) - y0);
    return area2 <= 0;
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

// 贴图颜色 × 光照亮度 = 最终颜色。
// 白色 255 × 亮度 0.5 = 128，等价于「这张贴图被照暗了一半」。
//
// ⚠ 第 13 步：alpha 通道要**原样保留**，不能再写死 255。
//    之前所有贴图都是不透明的，把 a 拍成 255 没人看得出来；
//    从这一步起贴图会自带 alpha（半透明的玻璃、镂空的栅栏），
//    光照只该压暗 RGB，跟透明度无关 —— 在这里把 a 抹掉，
//    后面 Alpha 测试和颜色混合拿到的就永远是 255，两个开关全部失效。
Color modulate(const Color& c, float light)
{
    auto f = [light](uint8_t v) -> uint8_t {
        const float r = static_cast<float>(v) * light;
        return static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, r)));
    };
    return Color(f(c.r), f(c.g), f(c.b), c.a);
}

}  // namespace

// ---- 平面着色版 ----
void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0,
                        int x1, int y1,
                        int x2, int y2,
                        uint32_t color)
{
    // 背面剔除（第 11 步）：背对相机的三角形整块跳过，一个像素都不填
    if (back_facing(x0, y0, x1, y1, x2, y2)) return;

    // ---- 包围盒（顺带裁剪到画布内）----
    const int min_x = std::max(0, std::min({x0, x1, x2}));
    const int min_y = std::max(0, std::min({y0, y1, y2}));
    const int max_x = std::min(fb.width()  - 1, std::max({x0, x1, x2}));
    const int max_y = std::min(fb.height() - 1, std::max({y0, y1, y2}));

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {

            // 三条边各算一次边函数（= 2 × 对应子三角形的有向面积）
            const long long w0 = edge_cross(x0, y0, x1, y1, x, y);  // 边 V0→V1
            const long long w1 = edge_cross(x1, y1, x2, y2, x, y);  // 边 V1→V2
            const long long w2 = edge_cross(x2, y2, x0, y0, x, y);  // 边 V2→V0

            // 三个边函数同号 → 在三角形内。
            // （背面已经在函数开头整块剔除；这一层判断本身跟绕序无关，
            //  正负两种都接受，留着更省心）
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
    // 背面剔除（第 11 步）
    if (back_facing(x0, y0, x1, y1, x2, y2)) return;

    const int min_x = std::max(0, std::min({x0, x1, x2}));
    const int min_y = std::max(0, std::min({y0, y1, y2}));
    const int max_x = std::min(fb.width()  - 1, std::max({x0, x1, x2}));
    const int max_y = std::min(fb.height() - 1, std::max({y0, y1, y2}));

    const SDL_PixelFormat* fmt = fb.format();

    RGBf C0, C1, C2;
    unpack3(c0, c1, c2, fmt, C0, C1, C2);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {

            const long long w0 = edge_cross(x0, y0, x1, y1, x, y);
            const long long w1 = edge_cross(x1, y1, x2, y2, x, y);
            const long long w2 = edge_cross(x2, y2, x0, y0, x, y);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (w0 <= 0 && w1 <= 0 && w2 <= 0)) {

                // w0+w1+w2 = 2 × 三角形面积，是个恒定值。
                // 除以它，w 就变成了重心坐标（三个权重，和为 1）。
                const long long sum = w0 + w1 + w2;
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
    // 背面剔除（第 11 步）
    if (back_facing(x0, y0, x1, y1, x2, y2)) return;

    const int min_x = std::max(0, std::min({x0, x1, x2}));
    const int min_y = std::max(0, std::min({y0, y1, y2}));
    const int max_x = std::min(fb.width()  - 1, std::max({x0, x1, x2}));
    const int max_y = std::min(fb.height() - 1, std::max({y0, y1, y2}));

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {

            const long long w0 = edge_cross(x0, y0, x1, y1, x, y);
            const long long w1 = edge_cross(x1, y1, x2, y2, x, y);
            const long long w2 = edge_cross(x2, y2, x0, y0, x, y);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (w0 <= 0 && w1 <= 0 && w2 <= 0)) {

                // 重心坐标插值算深度
                const long long sum = w0 + w1 + w2;
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

// ---- 带深度测试 + 三颜色（平滑着色）版 ----
void draw_triangle_depth(PixelBuffer& fb,
                         int x0, int y0, float z0, uint32_t c0,
                         int x1, int y1, float z1, uint32_t c1,
                         int x2, int y2, float z2, uint32_t c2)
{
    // 背面剔除（第 11 步）
    if (back_facing(x0, y0, x1, y1, x2, y2)) return;

    const int min_x = std::max(0, std::min({x0, x1, x2}));
    const int min_y = std::max(0, std::min({y0, y1, y2}));
    const int max_x = std::min(fb.width()  - 1, std::max({x0, x1, x2}));
    const int max_y = std::min(fb.height() - 1, std::max({y0, y1, y2}));

    const SDL_PixelFormat* fmt = fb.format();
    RGBf C0, C1, C2;
    unpack3(c0, c1, c2, fmt, C0, C1, C2);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {

            const long long w0 = edge_cross(x0, y0, x1, y1, x, y);
            const long long w1 = edge_cross(x1, y1, x2, y2, x, y);
            const long long w2 = edge_cross(x2, y2, x0, y0, x, y);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (w0 <= 0 && w1 <= 0 && w2 <= 0)) {

                const long long sum = w0 + w1 + w2;
                if (sum == 0) continue;

                const float inv = 1.0f / static_cast<float>(sum);
                const float z = (w1 * z0 + w2 * z1 + w0 * z2) * inv;

                if (z < fb.depth_at(x, y)) {
                    // 用重心坐标插值颜色
                    const float a0 = w1 * inv;   // V0 的权重
                    const float a1 = w2 * inv;   // V1 的权重
                    const float a2 = w0 * inv;   // V2 的权重
                    fb.put_pixel(x, y,
                        pack_rgb(mix_rgb(a0, C0, a1, C1, a2, C2), fmt));
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

// ============================================================
//  带纹理 + 透视校正的三角形填充
// ============================================================

// 透视校正到底在修什么？
//
// 屏幕上一条等长的线段，映射回三维空间里**并不等长**：离相机近的那段
// 在屏幕上只占一小截，实际却很长。所以像 UV 这种「贴在三维表面上的属性」，
// 拿屏幕空间的重心坐标直接线性插值，算出来的位置是错的 ——
// 表现就是在斜着看的面上，贴图被「往远处拉」，格子被拉成弧形。
//
// 数学上能证明：**1/w 在屏幕空间是线性的**，而 u/w 也是。
// 所以正确做法是：
//     ① 在屏幕空间线性插值 u/w、v/w、1/w 这三个量（这一步合法）
//     ② 再除回来：u = interp(u/w) / interp(1/w)
// 这就是「透视校正插值」。多出来的代价只是每像素两次乘加和一次除法。
void draw_triangle_textured(PixelBuffer& fb,
                            const ScreenVertex& v0,
                            const ScreenVertex& v1,
                            const ScreenVertex& v2,
                            const Texture& texture,
                            const DrawOptions& options)
{
    // 背面剔除（第 11 步）：先整块判一次，省掉整个包围盒的填像素开销。
    // 这一版走的是「裁剪后」的三角形，clipper 的扇形三角化不改变绕序，
    // 所以裁剪出来的碎块照样能正确判正反。
    if (back_facing(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y)) return;

    // ---- 包围盒（顺带裁剪到画布内）----
    const int min_x = std::max(0, std::min({v0.x, v1.x, v2.x}));
    const int min_y = std::max(0, std::min({v0.y, v1.y, v2.y}));
    const int max_x = std::min(fb.width()  - 1, std::max({v0.x, v1.x, v2.x}));
    const int max_y = std::min(fb.height() - 1, std::max({v0.y, v1.y, v2.y}));

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {

            const long long w0 = edge_cross(v0.x, v0.y, v1.x, v1.y, x, y);  // 边 V0→V1
            const long long w1 = edge_cross(v1.x, v1.y, v2.x, v2.y, x, y);  // 边 V1→V2
            const long long w2 = edge_cross(v2.x, v2.y, v0.x, v0.y, x, y);  // 边 V2→V0

            if (!((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                  (w0 <= 0 && w1 <= 0 && w2 <= 0))) {
                continue;
            }

            const long long sum = w0 + w1 + w2;
            if (sum == 0) continue;     // 退化三角形，跳过

            const float inv = 1.0f / static_cast<float>(sum);
            const float a0 = w1 * inv;  // V0 的权重
            const float a1 = w2 * inv;  // V1 的权重
            const float a2 = w0 * inv;  // V2 的权重

            // 深度：NDC 的 z 在屏幕空间本来就是线性的，直接插
            const float z = a0 * v0.z + a1 * v1.z + a2 * v2.z;

            // 深度测试照旧。注意：半透明物体**也**要做这一步 ——
            // 它虽然不写深度，但必须知道「自己前面有没有不透明的东西挡着」，
            // 被挡住的部分就不该混进来（否则会把藏在前景后面的玻璃画到前面来）。
            if (z >= fb.depth_at(x, y)) continue;   // 更远就不画

            // 亮度：0~1 的标量，跟随 uv 一起校正的影响很小，直接线性插值
            const float light = a0 * v0.light + a1 * v1.light + a2 * v2.light;

            float u, v;
            if (options.perspective_correct) {
                // 插值 1/w，以及 u/w、v/w —— 这三个在屏幕空间都是线性的
                const float iw = a0 * v0.invw + a1 * v1.invw + a2 * v2.invw;
                if (iw > 0.0f) {
                    const float uw = a0 * v0.u * v0.invw
                                   + a1 * v1.u * v1.invw
                                   + a2 * v2.u * v2.invw;
                    const float vw = a0 * v0.v * v0.invw
                                   + a1 * v1.v * v1.invw
                                   + a2 * v2.v * v2.invw;
                    u = uw / iw;    // 除回来，才是这个像素真正对应的 UV
                    v = vw / iw;
                } else {
                    // 理论上到不了这里：renderer 已经把 w 太小的顶点剔掉了
                    u = a0 * v0.u + a1 * v1.u + a2 * v2.u;
                    v = a0 * v0.v + a1 * v1.v + a2 * v2.v;
                }
            } else {
                // 对照实验用的错误做法：把 uv 当普通属性线性插值。
                // 看似合理，实际上是把「屏幕上的等距」当成了「三维空间里的等距」
                u = a0 * v0.u + a1 * v1.u + a2 * v2.u;
                v = a0 * v0.v + a1 * v1.v + a2 * v2.v;
            }

            const Color texel = options.bilinear ? texture.sample_bilinear(u, v)
                                                 : texture.sample_nearest(u, v);
            const Color src = modulate(texel, light);

            // ---- ③ 片元处理（第 13 步）----
            //
            // 顺序很讲究：Alpha 测试 → 混合 → 写深度，而且要在「深度测试之后、
            // 真正落笔之前」。理由：
            //   · 测试放在最前：一个被丢掉的片元连混合都不用算，白省一次读像素；
            //   · 混合必须在写深度之前：混合要读目标像素的旧颜色，先写深度不影响它，
            //     但先写颜色再读就晚了 —— 不过更重要的是下面这条；
            //   · 半透明物体不写深度：它只是「路过」这个像素，不拥有它。
            //     如果写了深度，后面更近的半透明片元会被它挡掉，玻璃就变成一块死色。

            // ① Alpha 测试：alpha 不达标的片元整个丢掉。
            //    表现就是「镂空」—— 贴图里 alpha=0 的地方真的成了洞，能看见后面。
            if (options.alpha_test &&
                !blend::alpha_test(src.a, options.alpha_threshold)) {
                continue;   // 不写颜色、不写深度：这个片元当作没来过
            }

            // ② 颜色混合：不覆盖，而是和这个像素已经画好的颜色（dst）加权平均。
            //    半透明物体靠它把自己「叠」在背景上。
            if (options.blend) {
                const Color dst = fb.get_pixel_color(x, y);
                fb.put_pixel(x, y, blend::blend_over(src, dst, src.a));
            } else {
                fb.put_pixel(x, y, src);
            }

            // ③ 写深度：由状态控制，半透明物体传 false。
            if (options.write_depth) {
                fb.set_depth(x, y, z);
            }
        }
    }
}
