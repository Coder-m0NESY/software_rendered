#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstdint>

#include "math/mat.hpp"
#include "math/vec.hpp"

class PixelBuffer;
class Texture;

// 渲染器：串联整条管线。
//
// 它认识矩阵和向量（math 层），也认识 PixelBuffer（core 层），
// 但**不认识 SDL**——开窗、事件、贴屏幕是 main.cpp 和 Window 的事。
//
// 目前做两件事：
//   线框渲染（draw_wireframe）：顶点 → 屏幕坐标 → draw_line
//   实心渲染（draw_filled）：  顶点 → 屏幕坐标 → draw_triangle_depth（带深度测试）
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

// 画一个实心模型（平滑着色版）：每个顶点带自己的颜色。
// colors 数组和 vertices 数组一一对应，长度相同。
// 三角形内部用重心坐标插值颜色。
void draw_filled_colored(PixelBuffer& fb,
                         const Mat44& mvp,
                         const Vec3* vertices, const uint32_t* colors,
                         int vertex_count,
                         const int (*triangles)[3], int triangle_count);

// 画一个「贴图 + 光照」的实心模型（第 10 步；第 11 步补了裁剪）。
//   uvs         和 vertices 一一对应，是每个顶点的贴图坐标
//   intensities 每个顶点的亮度 0~1（Lambert 算出来的），最终色 = 贴图 × 亮度
//
// 顶点变换和上面那个一样，多出来的是两件事：
//   ① 先做一次**近平面裁剪**（render/clipper），把跨过近平面的三角形切开，
//      切完的碎片再走透视除法——所以这里送进光栅化器的三角形一定都在近平面之内；
//   ② 算完透视除法后，把 1/w 也存下来交给光栅化器——透视校正就靠它。
void draw_filled_textured(PixelBuffer& fb,
                          const Mat44& mvp,
                          const Vec3* vertices, const Vec2* uvs,
                          const float* intensities,
                          int vertex_count,
                          const int (*triangles)[3], int triangle_count,
                          const Texture& texture,
                          bool perspective_correct,
                          bool bilinear);

}  // namespace renderer

#endif
