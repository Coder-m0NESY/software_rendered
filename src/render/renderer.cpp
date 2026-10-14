#include "renderer.hpp"

#include "core/pixelbuffer.hpp"
#include "core/texture.hpp"
#include "render/clipper.hpp"
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
    // 棱表 edges[2] → 三角形表 triangles[3]，draw_line → draw_triangle_depth。

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
        screen_z[i] = ndc.z;   // 存 NDC z，范围 [-1,1]，和深度初始值 +1.0 同空间
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

void renderer::draw_filled_colored(PixelBuffer& fb,
                                   const Mat44& mvp,
                                   const Vec3* vertices, const uint32_t* colors,
                                   int vertex_count,
                                   const int (*triangles)[3], int triangle_count)
{
    // 和 draw_filled 几乎一样，只是多了 colors 数组。
    // 存每个顶点的颜色引用（不用拷贝，数组里的指针指向 main 的 colors 数组）。

    constexpr int kMaxVertices = 1024;
    if (vertex_count > kMaxVertices) {
        vertex_count = kMaxVertices;
    }

    int screen_x[kMaxVertices];
    int screen_y[kMaxVertices];
    float screen_z[kMaxVertices];
    uint32_t vertex_color[kMaxVertices];
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
        screen_z[i] = ndc.z;
        vertex_color[i] = colors[i];
    }

    for (int t = 0; t < triangle_count; ++t) {
        const int a = triangles[t][0];
        const int b = triangles[t][1];
        const int c = triangles[t][2];
        if (!visible[a] || !visible[b] || !visible[c]) continue;
        draw_triangle_depth(fb,
                            screen_x[a], screen_y[a], screen_z[a], vertex_color[a],
                            screen_x[b], screen_y[b], screen_z[b], vertex_color[b],
                            screen_x[c], screen_y[c], screen_z[c], vertex_color[c]);
    }
}

void renderer::draw_filled_textured(PixelBuffer& fb,
                                    const Mat44& mvp,
                                    const Vec3* vertices, const Vec2* uvs,
                                    const float* intensities,
                                    int vertex_count,
                                    const int (*triangles)[3], int triangle_count,
                                    const Texture& texture,
                                    bool perspective_correct,
                                    bool bilinear)
{
    // =======================================================================
    //  第 11 步的关键改动：把「整块丢弃」换成「近平面裁剪」。
    //
    //  改造前（第 10 步）：
    //      clip.w < 1e-6 → visible[i] = false → 凡是用到这个顶点的三角形
    //      整块不画。后果就是「地板不敢铺大」：只要有一个角跑到相机背后，
    //      连着它的那块三角形就会整块消失，画面缺一块。
    //
    //  改造后：
    //      顶点一个都不丢，全部以**裁剪空间**坐标留着；
    //      三角形送进 clipper，让近平面切一刀，切出来的碎块照常画。
    //      于是相机贴脸时画面只是被正确地切掉一部分，不会有穿屏的三角形飞出来。
    //
    //  顺序仍然是老规矩：裁剪 → 透视除法 → 视口。裁剪必须在前，
    //  因为 w ≤ 0 的点做完除法会把坐标翻到屏幕另一侧。
    // =======================================================================

    constexpr int kMaxVertices = 1024;
    if (vertex_count > kMaxVertices) {
        vertex_count = kMaxVertices;
    }

    // ---- ① 顶点变换：只乘到裁剪空间，不除法、不丢点 ----
    ClipVertex clip_vertices[kMaxVertices];
    for (int i = 0; i < vertex_count; ++i) {
        clip_vertices[i].position = mvp * vertices[i];
        clip_vertices[i].uv       = uvs[i];
        clip_vertices[i].light    = intensities[i];
    }

    // ---- ② 逐三角形：裁剪 → ③ 透视除法 + 视口 → ④ 光栅化 ----
    for (int t = 0; t < triangle_count; ++t) {
        const ClipVertex input[3] = {
            clip_vertices[triangles[t][0]],
            clip_vertices[triangles[t][1]],
            clip_vertices[triangles[t][2]],
        };

        // 切一刀最多变出 2 个三角形，输出顶点紧挨着放：
        // 第 p 个三角形的三个顶点在 output[p*3 .. p*3+2]
        ClipVertex output[clipper::kMaxOutputTriangles * 3];
        const int piece_count = clipper::clip_near_plane(input, output);

        for (int p = 0; p < piece_count; ++p) {
            ScreenVertex sv[3];
            for (int k = 0; k < 3; ++k) {
                const ClipVertex& cv = output[p * 3 + k];

                // 裁剪保证了 w >= near > 0，所以这里的除法是安全的
                const Vec4 ndc = cv.position.perspectiveDivide();
                viewport(ndc.x, ndc.y, fb.width(), fb.height(),
                         sv[k].x, sv[k].y);

                sv[k].z     = ndc.z;                    // 深度测试用
                sv[k].invw  = 1.0f / cv.position.w;     // 透视校正的关键：存 1/w
                sv[k].u     = cv.uv.x;
                sv[k].v     = cv.uv.y;
                sv[k].light = cv.light;
            }

            draw_triangle_textured(fb, sv[0], sv[1], sv[2],
                                   texture, perspective_correct, bilinear);
        }
    }
}
