#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstdint>

#include "math/mat.hpp"
#include "math/vec.hpp"

class PixelBuffer;

// 渲染器：串联整条管线。
//
// 它认识矩阵和向量（math 层），也认识 PixelBuffer（core 层），
// 但**不认识 SDL**——开窗、事件、贴屏幕是 main.cpp 和 Window 的事。
//
// 目前做两件事：
//   线框渲染（draw_wireframe）：顶点 → 屏幕坐标 → draw_line
//   实心渲染（draw_filled）：  顶点 → 屏幕坐标 → draw_triangle
namespace renderer {

// NDC [-1,1] → 屏幕像素坐标。
// 注意 y 要翻转：NDC 里 y 向上为正，屏幕上 y 向下为正。
void viewport(float ndc_x, float ndc_y,
              int width, int height,
              int& out_sx, int& out_sy);

// 画一个线框模型：顶点数组 + 棱表（每行是两个顶点下标）。
// color 是 0xAARRGGBB 的裸像素值。
void draw_wireframe(PixelBuffer& fb,
                    const Mat44& mvp,
                    const Vec3* vertices, int vertex_count,
                    const int (*edges)[2], int edge_count,
                    uint32_t color);

// 画一个实心模型：顶点数组 + 三角形表（每行是三个顶点下标）。
// color 是 0xAARRGGBB 的裸像素值（平面着色，整个模型一个颜色）。
void draw_filled(PixelBuffer& fb,
                 const Mat44& mvp,
                 const Vec3* vertices, int vertex_count,
                 const int (*triangles)[3], int triangle_count,
                 uint32_t color);

}  // namespace renderer

#endif
