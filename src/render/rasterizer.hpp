#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP

#include <cstdint>

class PixelBuffer;
class Color;

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

#endif
