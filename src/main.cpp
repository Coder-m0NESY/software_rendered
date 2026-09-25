// 学习路线第 7 步：实心立方体。
//
// 在第 6 步（线框）的基础上，把 12 条棱换成 12 个三角形：
//   立方体 6 个面 × 每面 2 个三角形 = 12 个三角形
//   draw_line → draw_triangle（边函数光栅化）
// 验收：立方体变实心，转起来能看到面（后面的盖住前面的是正常现象，第 8 步治）。
//
// 【为什么必须保留 #include <SDL.h>】
// CMakeLists 链接了 SDL2main，它提供的 WinMain 会去调用被改名后的 SDL_main。
// SDL.h 里的宏会把 main 改名成 SDL_main，两者才对得上。
// 去掉这个 include 会直接链接失败：undefined reference to `SDL_main'。
#include <SDL.h>

#include "core/color.hpp"
#include "core/pixelbuffer.hpp"
#include "core/window.hpp"
#include "math/mat.hpp"
#include "math/vec.hpp"
#include "render/renderer.hpp"

#include <cstdio>

int main(int argc, char* argv[])
{
    // ---- 1. 窗口和画布 ----
    constexpr int kWidth  = 800;
    constexpr int kHeight = 600;

    Window window("SoftRenderer - filled cube", kWidth, kHeight);
    if (window.closed()) {
        std::printf("window creation failed\n");
        return 1;
    }

    PixelBuffer buffer(kWidth, kHeight);

    if (buffer.pitch() != buffer.width() * 4) {
        std::printf("pitch (%d) != width*4 (%d), cannot present directly\n",
                    buffer.pitch(), buffer.width() * 4);
        return 1;
    }

    // ---- 2. 立方体：8 个顶点 + 12 个三角形 ----
    // 顶点编号规律：0~3 是 z=-1 的远面四个角，4~7 是 z=+1 的近面四个角。
    const Vec3 vertices[8] = {
        Vec3(-1.0f, -1.0f, -1.0f),   // 0  远面左下
        Vec3( 1.0f, -1.0f, -1.0f),   // 1  远面右下
        Vec3( 1.0f,  1.0f, -1.0f),   // 2  远面右上
        Vec3(-1.0f,  1.0f, -1.0f),   // 3  远面左上
        Vec3(-1.0f, -1.0f,  1.0f),   // 4  近面左下
        Vec3( 1.0f, -1.0f,  1.0f),   // 5  近面右下
        Vec3( 1.0f,  1.0f,  1.0f),   // 6  近面右上
        Vec3(-1.0f,  1.0f,  1.0f),   // 7  近面左上
    };

    // 三角形表：每行三个顶点下标，组成一个三角形。
    // 立方体 6 个面 × 每面 2 个三角形 = 12 个。
    // 绕序：从立方体外面看是逆时针（CCW）。
    //       如果发现面全消失了（面积全为负被跳过），把每行后两个数交换即可。
    const int triangles[12][3] = {
        // 远面 z=-1（顶点 0,1,2,3）从 -z 方向看
        {0, 1, 2}, {0, 2, 3},
        // 近面 z=+1（顶点 4,5,6,7）从 +z 方向看
        {4, 6, 5}, {4, 7, 6},
        // 底面 y=-1（顶点 0,1,4,5）从 -y 方向看
        {0, 1, 5}, {0, 5, 4},
        // 顶面 y=+1（顶点 2,3,6,7）从 +y 方向看
        {2, 6, 7}, {2, 7, 3},
        // 左面 x=-1（顶点 0,3,4,7）从 -x 方向看
        {0, 4, 7}, {0, 7, 3},
        // 右面 x=+1（顶点 1,2,5,6）从 +x 方向看
        {1, 2, 6}, {1, 6, 5},
    };

    // ---- 3. 相机和投影（整段程序不变，放循环外） ----
    constexpr float kPi = 3.14159265358979f;
    const Mat44 view = Mat44::lookAt(Vec3(0.0f, 0.0f, 3.0f),
                                     Vec3(0.0f, 0.0f, 0.0f),
                                     Vec3(0.0f, 1.0f, 0.0f));
    const Mat44 projection = Mat44::perspective(kPi / 3.0f,
                                                static_cast<float>(kWidth) /
                                                    static_cast<float>(kHeight),
                                                0.1f, 100.0f);

    // ---- 4. 主循环 ----
    while (!window.closed()) {
        window.poll_events();

        buffer.clear(Color(0, 0, 0));   // 清屏（clear 内部已经调 clear_depth）

        // 绕 Y 轴旋转
        const float angle = static_cast<float>(SDL_GetTicks64()) * 0.001f;
        const Mat44 model = Mat44::rotateY(angle);
        const Mat44 mvp = projection * view * model;

        // 12 个三角形填满像素 → 立方体变实心
        renderer::draw_filled(buffer, mvp,
                              vertices, 8,
                              triangles, 12,
                              Color(200, 200, 200).to_pixel());  // 灰色

        window.present(static_cast<const std::uint32_t*>(
            buffer.surface()->pixels));
    }

    return 0;
}
