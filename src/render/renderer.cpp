#include "renderer.hpp"

#include "core/pixelbuffer.hpp"
#include "render/rasterizer.hpp"

// ===========================================================================
//  renderer 的实现。
//  管线的顺序跟教程里那张图一致：
//    顶点 × MVP → 裁剪坐标 → 透视除法(NDC) → 视口变换 → 屏幕坐标 → 画线/填三角形
// ===========================================================================

void renderer::viewport(float ndc_x, float ndc_y,
                        int width, int height,
                        int& out_sx, int& out_sy)
{
    // [-1,1] → [0,width] / [0,height]
    // x：-1 是最左，+1 是最右
    // y：NDC 里 +1 是"上"，屏幕上 y=0 才是"上"，所以要 1 - ndc.y
    out_sx = static_cast<int>((ndc_x + 1.0f) * 0.5f * static_cast<float>(width));
    out_sy = static_cast<int>((1.0f - ndc_y) * 0.5f * static_cast<float>(height));
}

void renderer::draw_wireframe(PixelBuffer& fb,
                              const Mat44& mvp,
                              const Vec3* vertices, int vertex_count,
                              const int (*edges)[2], int edge_count,
                              uint32_t color)
{
    constexpr int kMaxVertices = 1024;
    if (vertex_count > kMaxVertices) {
        vertex_count = kMaxVertices;
    }

    int screen_x[kMaxVertices];
    int screen_y[kMaxVertices];
    bool visible[kMaxVertices];

    for (int i = 0; i < vertex_count; ++i) {
        const Vec4 clip = mvp * vertices[i];
        if (clip.w < 1e-6f) {
            visible[i] = false;
            continue;
        }
        visible[i] = true;
        const Vec4 ndc = clip.perspectiveDivide();
        viewport(ndc.x, ndc.y, fb.width(), fb.height(),
                 screen_x[i], screen_y[i]);
    }

    for (int e = 0; e < edge_count; ++e) {
        const int a = edges[e][0];
        const int b = edges[e][1];
        if (!visible[a] || !visible[b]) continue;
        draw_line(fb, screen_x[a], screen_y[a], screen_x[b], screen_y[b], color);
    }
}

void renderer::draw_filled(PixelBuffer& fb,
                          const Mat44& mvp,
                          const Vec3* vertices, int vertex_count,
                          const int (*triangles)[3], int triangle_count,
                          uint32_t color)
{
    // 和 draw_wireframe 一样：先把所有顶点算成屏幕坐标，再按索引表画。
    // 第一个循环（顶点变换）和 draw_wireframe 完全相同，只有第二个循环不同：
    // 棱表 edges[2] → 三角形表 triangles[3]，draw_line → draw_triangle。

    constexpr int kMaxVertices = 1024;
    if (vertex_count > kMaxVertices) {
        vertex_count = kMaxVertices;
    }

    int screen_x[kMaxVertices];
    int screen_y[kMaxVertices];
    float screen_z[kMaxVertices];
    bool visible[kMaxVertices];

    for (int i = 0; i < vertex_count; ++i) {
        const Vec4 clip = mvp * vertices[i];
        if (clip.w < 1e-6f) {
            visible[i] = false;
            continue;
        }
        visible[i] = true;
        const Vec4 ndc = clip.perspectiveDivide();
        viewport(ndc.x, ndc.y, fb.width(), fb.height(),
                 screen_x[i], screen_y[i]);
        screen_z[i] = clip.z;   // 存 clip.z，线性可插值
    }

    // 按三角形表，每行三个顶点下标，填一个三角形
    for (int t = 0; t < triangle_count; ++t) {
        const int a = triangles[t][0];
        const int b = triangles[t][1];
        const int c = triangles[t][2];
        if (!visible[a] || !visible[b] || !visible[c]) continue;
        draw_triangle_depth(fb,
                            screen_x[a], screen_y[a], screen_z[a],
                            screen_x[b], screen_y[b], screen_z[b],
                            screen_x[c], screen_y[c], screen_z[c],
                            color);
    }
}
