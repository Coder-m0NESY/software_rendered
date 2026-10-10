// 学习路线第 9 步：Lambert 光照 + 鼠标转相机。
//
// 把 8 顶点立方体升级成 36 顶点（每个面两个三角形各带独立法线），
// 用 Lambert 公式算每个顶点的亮度，三角形内部用重心坐标插值。
// 相机用鼠标拖动控制（左键），保留时间驱动的保底旋转。
//
// 验收：立方体不同面亮度不同，鼠标拖动能转视角，深度测试正常。
#include <SDL.h>

#include "core/color.hpp"
#include "core/pixelbuffer.hpp"
#include "core/window.hpp"
#include "math/mat.hpp"
#include "math/vec.hpp"
#include "render/lighting.hpp"
#include "render/renderer.hpp"
#include "scene/vertex.hpp"

#include <algorithm>   // std::clamp
#include <cmath>
#include <cstdio>

int main(int argc, char* argv[])
{
    // ---- 1. 窗口和画布 ----
    constexpr int kWidth  = 800;
    constexpr int kHeight = 600;

    Window window("SoftRenderer - lit cube", kWidth, kHeight);
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

    // ---- 2. 36 顶点立方体：每面 6 个顶点（2 三角形 × 3 顶点），各带独立法线 ----
    // 每个面的两个三角形共享位置但不共享法线——因为不同面法线不一样。
    // 绕序约定：从立方体外面看是逆时针（CCW）。
    const Vertex vertices[36] = {
        // 远面 z=-1，法线 (0,0,-1)，从 -z 方向看
        { Vec3(-1,-1,-1), Vec3(0,0,-1) }, { Vec3( 1,-1,-1), Vec3(0,0,-1) }, { Vec3( 1, 1,-1), Vec3(0,0,-1) },
        { Vec3(-1,-1,-1), Vec3(0,0,-1) }, { Vec3( 1, 1,-1), Vec3(0,0,-1) }, { Vec3(-1, 1,-1), Vec3(0,0,-1) },
        // 近面 z=+1，法线 (0,0,+1)，从 +z 方向看
        { Vec3(-1,-1, 1), Vec3(0,0, 1) }, { Vec3(-1, 1, 1), Vec3(0,0, 1) }, { Vec3( 1, 1, 1), Vec3(0,0, 1) },
        { Vec3(-1,-1, 1), Vec3(0,0, 1) }, { Vec3( 1, 1, 1), Vec3(0,0, 1) }, { Vec3( 1,-1, 1), Vec3(0,0, 1) },
        // 底面 y=-1，法线 (0,-1,0)，从 -y 方向看
        { Vec3(-1,-1,-1), Vec3(0,-1,0) }, { Vec3(-1,-1, 1), Vec3(0,-1,0) }, { Vec3( 1,-1, 1), Vec3(0,-1,0) },
        { Vec3(-1,-1,-1), Vec3(0,-1,0) }, { Vec3( 1,-1, 1), Vec3(0,-1,0) }, { Vec3( 1,-1,-1), Vec3(0,-1,0) },
        // 顶面 y=+1，法线 (0,+1,0)，从 +y 方向看
        { Vec3(-1, 1,-1), Vec3(0, 1,0) }, { Vec3( 1, 1, 1), Vec3(0, 1,0) }, { Vec3(-1, 1, 1), Vec3(0, 1,0) },
        { Vec3(-1, 1,-1), Vec3(0, 1,0) }, { Vec3( 1, 1,-1), Vec3(0, 1,0) }, { Vec3( 1, 1, 1), Vec3(0, 1,0) },
        // 左面 x=-1，法线 (-1,0,0)，从 -x 方向看
        { Vec3(-1,-1,-1), Vec3(-1,0,0) }, { Vec3(-1,-1, 1), Vec3(-1,0,0) }, { Vec3(-1, 1, 1), Vec3(-1,0,0) },
        { Vec3(-1,-1,-1), Vec3(-1,0,0) }, { Vec3(-1, 1, 1), Vec3(-1,0,0) }, { Vec3(-1, 1,-1), Vec3(-1,0,0) },
        // 右面 x=+1，法线 (+1,0,0)，从 +x 方向看
        { Vec3( 1,-1,-1), Vec3( 1,0,0) }, { Vec3( 1, 1, 1), Vec3( 1,0,0) }, { Vec3( 1,-1, 1), Vec3( 1,0,0) },
        { Vec3( 1,-1,-1), Vec3( 1,0,0) }, { Vec3( 1, 1,-1), Vec3( 1,0,0) }, { Vec3( 1, 1, 1), Vec3( 1,0,0) },
    };

    // 三角形表：连续下标，每三个一组组成一个三角形。
    // 因为 36 顶点每 6 个属于一个面的两个三角形，这里就是简单的 0,1,2,3,4,5,...33,34,35。
    const int triangles[12][3] = {
        { 0, 1, 2}, { 3, 4, 5},   // 远面
        { 6, 7, 8}, { 9,10,11},   // 近面
        {12,13,14}, {15,16,17},   // 底面
        {18,19,20}, {21,22,23},   // 顶面
        {24,25,26}, {27,28,29},   // 左面
        {30,31,32}, {33,34,35},   // 右面
    };

    // ---- 3. 光源 + 投影（放循环外，一帧一帧复用） ----
    constexpr float kPi = 3.14159265358979f;
    const Color kBaseColor(255, 255, 255, 255);   // 立方体基础色（白色，亮度由光照决定）
    const Vec3  kLightPos(3.0f, 4.0f, 2.0f);      // 光源在世界空间的位置

    const Mat44 projection = Mat44::perspective(kPi / 3.0f,
                                                static_cast<float>(kWidth) /
                                                    static_cast<float>(kHeight),
                                                0.1f, 100.0f);

    // 主循环外的状态：相机的 yaw（水平角）和 pitch（俯仰角）
    float yaw   = 0.0f;     // 绕 Y 轴：0 朝 +Z
    float pitch = 0.0f;     // 绕 X 轴

    // 每帧算好的顶点颜色缓存
    uint32_t vertex_colors[36];

    // 位置数组：Vertex 数组内存里 position 和 normal 交替排列，
    // 而 renderer 需要一串连续的 Vec3，所以单独拷一份（初始化一次就够）。
    Vec3 positions[36];
    for (int i = 0; i < 36; ++i) {
        positions[i] = vertices[i].position;
    }

    // ---- 4. 主循环 ----
    while (!window.closed()) {
        window.poll_events();

        buffer.clear(Color(0, 0, 0));

        // ---- 相机：鼠标拖动更新 yaw/pitch ----
        yaw   += window.mouse_dx() * 0.005f;
        pitch -= window.mouse_dy() * 0.005f;     // 向下拖 = 相机朝下看，pitch 减小

        // 保底自动旋转（鼠标不动时也在转，防止静止看不到效果）
        // yaw += 0.002f;  // 已注释：相机不自动转，只让模型自己转

        // pitch 夹紧在 [-π/3, π/3]，防止相机翻到地板下面
        pitch = std::clamp(pitch, -kPi / 3.0f, kPi / 3.0f);

        // 球面坐标 → 相机位置，距离原点固定 3
        const float r = 3.0f;
        const float eye_x = r * std::cos(pitch) * std::sin(yaw);
        const float eye_y = r * std::sin(pitch);
        const float eye_z = r * std::cos(pitch) * std::cos(yaw);

        const Mat44 view = Mat44::lookAt(Vec3(eye_x, eye_y, eye_z),
                                         Vec3(0.0f, 0.0f, 0.0f),
                                         Vec3(0.0f, 1.0f, 0.0f));

        // ---- 模型：时间驱动绕 Y 轴转 ----
        const float model_angle = static_cast<float>(SDL_GetTicks64()) * 0.001f;
        const Mat44 model = Mat44::rotateY(model_angle);

        // ---- Lambert 光照：对每个顶点算亮度 ----
        // 法线变换：M 是纯旋转（没有非均匀缩放），所以 M 的逆转置 = M。
        // 直接乘 M 就行；如果以后 M 带非均匀缩放，这里必须换成逆转置。
        for (int i = 0; i < 36; ++i) {
            const Vec3 world_pos    = (model * vertices[i].position).xyz();
            const Vec3 world_normal = model.transformDirection(vertices[i].normal);
            const Vec3 light_dir = normalize(kLightPos - world_pos);
            vertex_colors[i] = compute_lambert(world_normal, light_dir, kBaseColor).to_pixel();
        }

        const Mat44 mvp = projection * view * model;

        // ---- 渲染 ----
        renderer::draw_filled_colored(buffer, mvp,
                                      positions, vertex_colors,
                                      36, triangles, 12);

        window.present(static_cast<const std::uint32_t*>(
            buffer.surface()->pixels));
    }

    return 0;
}
