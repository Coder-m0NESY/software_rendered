#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP

#include <cstdint>

class PixelBuffer;
class Color;
class Texture;

// 光栅化器：给我屏幕像素坐标，我把中间的像素全部填满。
//
// 它只认识 PixelBuffer，不知道矩阵、相机、模型是什么东西——
// 顶点怎么算出来是 renderer 的事，这里只管"点连成线、三角形填满"。
//
// 所有坐标都是像素坐标，原点在画布左上角，x 向右、y 向下。
// 越界的点由 PixelBuffer::put_pixel 自己丢弃，这里不做裁剪。

// ---- Bresenham 直线 ----

void draw_line(PixelBuffer& fb,
               int x0, int y0, int x1, int y1,
               uint32_t color);

void draw_line(PixelBuffer& fb,
               int x0, int y0, int x1, int y1,
               const Color& color);

// ---- 边函数三角形填充 ----

// 平面着色：整个三角形一个颜色
void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0, int x1, int y1, int x2, int y2,
                        uint32_t color);

void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0, int x1, int y1, int x2, int y2,
                        const Color& color);

// 平滑着色：三个顶点各一个颜色，用重心坐标插值
void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0, int x1, int y1, int x2, int y2,
                        uint32_t c0, uint32_t c1, uint32_t c2);

void draw_triangle_edge(PixelBuffer& fb,
                        int x0, int y0, int x1, int y1, int x2, int y2,
                        const Color& c0, const Color& c1, const Color& c2);

// ---- 带深度测试的三角形填充 ----

void draw_triangle_depth(PixelBuffer& fb,
                         int x0, int y0, float z0,
                         int x1, int y1, float z1,
                         int x2, int y2, float z2,
                         uint32_t color);

// 带深度测试 + 三个顶点各带一个颜色（平滑着色）
void draw_triangle_depth(PixelBuffer& fb,
                         int x0, int y0, float z0, uint32_t c0,
                         int x1, int y1, float z1, uint32_t c1,
                         int x2, int y2, float z2, uint32_t c2);

// ---- 通用入口：默认走边函数法 ----

void draw_triangle(PixelBuffer& fb,
                   int x0, int y0, int x1, int y1, int x2, int y2,
                   uint32_t color);

void draw_triangle(PixelBuffer& fb,
                   int x0, int y0, int x1, int y1, int x2, int y2,
                   const Color& color);

void draw_triangle(PixelBuffer& fb,
                   int x0, int y0, int x1, int y1, int x2, int y2,
                   uint32_t c0, uint32_t c1, uint32_t c2);

void draw_triangle(PixelBuffer& fb,
                   int x0, int y0, int x1, int y1, int x2, int y2,
                   const Color& c0, const Color& c1, const Color& c2);

// ---- 带纹理 + 透视校正的三角形填充 ----

// 光照 + 贴图模式下，一个「已经算好屏幕坐标」的顶点。
// 前面几版都是把参数一个个列出来（x0, y0, z0, c0, x1, ...），
// 到这一版每个顶点要带 7 个数，二十多个参数没法看，所以打包成一个结构体。
struct ScreenVertex {
    int   x = 0;            // 屏幕像素坐标，原点在左上角
    int   y = 0;
    float z = 0.0f;         // NDC 的 z，深度测试用（越小越近）
    float invw = 1.0f;      // 1 / clip.w，透视校正的分母
    float u = 0.0f;         // 贴图坐标
    float v = 0.0f;
    float light = 1.0f;     // 这个顶点的亮度 0~1（Lambert 算出来的）
};

// 每个顶点带 invw / uv / 亮度，三角形内部插值后采样贴图。
//
// 只有 uv 必须做透视校正：
//     u = interp(u / w) / interp(1 / w)
// 其余属性（1/w、亮度）在屏幕空间直接线性插值就是对的 ——
// 1/w 本身在屏幕空间是线性的，亮度校正与否肉眼看不出差别。
//
// perspective_correct = false 时退回「朴素做法」（uv 也当普通属性线性插值），
// 专门留在代码里做对照实验，让你亲眼看一次扭曲长什么样。
void draw_triangle_textured(PixelBuffer& fb,
                            const ScreenVertex& v0,
                            const ScreenVertex& v1,
                            const ScreenVertex& v2,
                            const Texture& texture,
                            bool perspective_correct = true,
                            bool bilinear = false);

#endif
