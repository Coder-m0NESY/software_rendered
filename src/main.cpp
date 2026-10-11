// 学习路线第 10 步：纹理 + 透视校正。
//
// 在第 9 步（Lambert 光照 + 鼠标相机）的基础上：
//   ① 给 Vertex 加上 UV，并新建 core/texture（棋盘格 + 最近邻/双线性采样）
//   ② 光栅化时用「uv/w 与 1/w 插值」做透视校正采样
//   ③ 最终颜色 = 贴图颜色 × 光照亮度
//
// 键盘（做对照实验用）：
//   T  切换透视校正 开/关  —— 关掉看一眼真实的扭曲
//   B  切换采样方式 双线性/最近邻
//   鼠标左键拖动  转相机
//
// 验收：棋盘格近大远小、不扭曲；按 T 关掉校正后，斜面上的格子明显被拉歪。
#include <SDL.h>

#include "core/color.hpp"
#include "core/pixelbuffer.hpp"
#include "core/texture.hpp"
#include "core/window.hpp"
#include "io/image.hpp"
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

    Window window("SoftRenderer - textured cube", kWidth, kHeight);
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

    // ---- 2. 36 顶点立方体：每面 6 个顶点（2 三角形 × 3 顶点），各带独立法线、UV ----
    // 每个面的两个三角形共享位置但不共享法线——因为不同面法线不一样。
    // 绕序约定：从立方体外面看是逆时针（CCW）。
    //
    // UV 的取法：每个面都铺满一整张贴图，也就是该面的 4 个角对应
    // (0,0)(1,0)(0,1)(1,1)。同一张棋盘格贴在 6 个面上，转起来能看出
    // 每个面的格子大小都不一样（近大远小），也方便对比透视校正的效果。
    const Vertex vertices[36] = {
        // 远面 z=-1，法线 (0,0,-1)，从 -z 方向看
        { Vec3(-1,-1,-1), Vec3(0,0,-1), Vec2(0,1) }, { Vec3( 1,-1,-1), Vec3(0,0,-1), Vec2(1,1) }, { Vec3( 1, 1,-1), Vec3(0,0,-1), Vec2(1,0) },
        { Vec3(-1,-1,-1), Vec3(0,0,-1), Vec2(0,1) }, { Vec3( 1, 1,-1), Vec3(0,0,-1), Vec2(1,0) }, { Vec3(-1, 1,-1), Vec3(0,0,-1), Vec2(0,0) },
        // 近面 z=+1，法线 (0,0,+1)，从 +z 方向看
        { Vec3(-1,-1, 1), Vec3(0,0, 1), Vec2(0,1) }, { Vec3(-1, 1, 1), Vec3(0,0, 1), Vec2(0,0) }, { Vec3( 1, 1, 1), Vec3(0,0, 1), Vec2(1,0) },
        { Vec3(-1,-1, 1), Vec3(0,0, 1), Vec2(0,1) }, { Vec3( 1, 1, 1), Vec3(0,0, 1), Vec2(1,0) }, { Vec3( 1,-1, 1), Vec3(0,0, 1), Vec2(1,1) },
        // 底面 y=-1，法线 (0,-1,0)，从 -y 方向看
        { Vec3(-1,-1,-1), Vec3(0,-1,0), Vec2(0,0) }, { Vec3(-1,-1, 1), Vec3(0,-1,0), Vec2(0,1) }, { Vec3( 1,-1, 1), Vec3(0,-1,0), Vec2(1,1) },
        { Vec3(-1,-1,-1), Vec3(0,-1,0), Vec2(0,0) }, { Vec3( 1,-1, 1), Vec3(0,-1,0), Vec2(1,1) }, { Vec3( 1,-1,-1), Vec3(0,-1,0), Vec2(1,0) },
        // 顶面 y=+1，法线 (0,+1,0)，从 +y 方向看
        { Vec3(-1, 1,-1), Vec3(0, 1,0), Vec2(0,0) }, { Vec3( 1, 1, 1), Vec3(0, 1,0), Vec2(1,1) }, { Vec3(-1, 1, 1), Vec3(0, 1,0), Vec2(0,1) },
        { Vec3(-1, 1,-1), Vec3(0, 1,0), Vec2(0,0) }, { Vec3( 1, 1,-1), Vec3(0, 1,0), Vec2(1,0) }, { Vec3( 1, 1, 1), Vec3(0, 1,0), Vec2(1,1) },
        // 左面 x=-1，法线 (-1,0,0)，从 -x 方向看
        { Vec3(-1,-1,-1), Vec3(-1,0,0), Vec2(0,1) }, { Vec3(-1,-1, 1), Vec3(-1,0,0), Vec2(1,1) }, { Vec3(-1, 1, 1), Vec3(-1,0,0), Vec2(1,0) },
        { Vec3(-1,-1,-1), Vec3(-1,0,0), Vec2(0,1) }, { Vec3(-1, 1, 1), Vec3(-1,0,0), Vec2(1,0) }, { Vec3(-1, 1,-1), Vec3(-1,0,0), Vec2(0,0) },
        // 右面 x=+1，法线 (+1,0,0)，从 +x 方向看
        { Vec3( 1,-1,-1), Vec3( 1,0,0), Vec2(0,1) }, { Vec3( 1, 1, 1), Vec3( 1,0,0), Vec2(1,0) }, { Vec3( 1,-1, 1), Vec3( 1,0,0), Vec2(1,1) },
        { Vec3( 1,-1,-1), Vec3( 1,0,0), Vec2(0,1) }, { Vec3( 1, 1,-1), Vec3( 1,0,0), Vec2(0,0) }, { Vec3( 1, 1, 1), Vec3( 1,0,0), Vec2(1,0) },
    };

    // 三角形表：连续下标，每三个一组组成一个三角形。
    const int triangles[12][3] = {
        { 0, 1, 2}, { 3, 4, 5},   // 远面
        { 6, 7, 8}, { 9,10,11},   // 近面
        {12,13,14}, {15,16,17},   // 底面
        {18,19,20}, {21,22,23},   // 顶面
        {24,25,26}, {27,28,29},   // 左面
        {30,31,32}, {33,34,35},   // 右面
    };

    // ---- 3. 贴图：优先读一张真实图片；读不到就退回代码生成的棋盘格 ----
    // assets/texture_brick.bmp 是 512×512 的石头砖墙，能平铺。
    // 为什么是 BMP：SDL2 自带 SDL_LoadBMP，不用额外依赖 SDL_image（见 io/image.hpp）。
    //
    // 失败就退回棋盘格，是为了「贴图丢了程序也能跑」—— 加载文件属于系统边界，
    // 这里必须处理，而不是相信文件一定在。控制台会打一行说明走了哪条路。
    //
    // 注意：load_texture_bmp 只在成功时才写 out，失败时 out 保持原样。
    // 所以这里先让它等于棋盘格，读盘失败就自动是对照用的棋盘格，不用另存一份。
    Texture texture = Texture::make_checkerboard(64, 8, Color(235, 235, 235), Color(50, 58, 78));
    if (io::load_texture_bmp("assets/texture_brick.bmp", texture)) {
        std::printf("[贴图] 已加载 assets/texture_brick.bmp（%d×%d）\n",
                    texture.width(), texture.height());
    } else {
        std::printf("[贴图] 加载失败，退回代码生成的棋盘格\n");
    }

    // ---- 4. 地板：一块 6×6 的大四边形（2 个三角形），把同一张贴图重复铺开 ----
    // 为什么非要加个地板：透视校正的扭曲在「斜着看的大平面」上最明显。
    // 立方体每个面跨度只有 2 个单位、又基本正对相机，光看立方体几乎看不出
    // 校正与不校正的差别；地板从近铺到远，一格的宽窄变化一眼就能看出来 ——
    // 这就是「近大远小」。
    //
    // UV 取「世界坐标 ÷ 2」：一个贴图周期对应 2 个世界单位，和立方体一个面
    // 同样大，于是地板和立方体的格子尺寸一致。UV 范围是 [-1.5, 1.5]，超出 [0,1]
    // 的部分靠 Texture 内部的 wrap 平铺出去 —— 这就是「重复平铺」（tiling）。
    //
    // ⚠ 尺寸不是随手定的：现在还没有做裁剪（那是第 11 步），
    // renderer 会把「有一个顶点跑到相机背后」的三角形整个丢掉，
    // 地板就会缺一块。要让地板永远完整，必须保证它离原点最远的角
    // （√(3² + 3² + 1²) ≈ 4.36）比相机到原点的距离（下面 r = 5）更近。
    // 这就是「为什么这一步的地板不敢铺大」的真正原因。
    const Vec3 floor_positions[4] = {
        Vec3(-3.0f, -1.0f, -3.0f), Vec3( 3.0f, -1.0f, -3.0f),
        Vec3( 3.0f, -1.0f,  3.0f), Vec3(-3.0f, -1.0f,  3.0f)
    };
    const Vec2 floor_uvs[4] = {
        Vec2(-1.5f, -1.5f), Vec2( 1.5f, -1.5f),
        Vec2( 1.5f,  1.5f), Vec2(-1.5f,  1.5f)
    };
    const Vec3 floor_normal(0.0f, 1.0f, 0.0f);
    const int  floor_triangles[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } };
    float      floor_lights[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // ---- 5. 投影 + 相机状态 ----
    constexpr float kPi = 3.14159265358979f;

    const Mat44 projection = Mat44::perspective(kPi / 3.0f,
                                                static_cast<float>(kWidth) /
                                                    static_cast<float>(kHeight),
                                                0.1f, 100.0f);

    float yaw   = 0.0f;     // 绕 Y 轴：0 朝 +Z
    float pitch = 0.0f;     // 绕 X 轴

    // 对照实验开关：T 切透视校正，B 切采样方式
    bool perspective_correct = true;
    bool bilinear            = false;

    // 每帧算好的顶点亮度缓存（0~1）
    float intensities[36];

    // position 和 normal 在 Vertex 数组里是交替排列的，而 renderer 要的是
    // 一串连续的 Vec3 / Vec2，所以各拷一份出来（初始化一次就够）。
    Vec3 positions[36];
    Vec2 uvs[36];
    for (int i = 0; i < 36; ++i) {
        positions[i] = vertices[i].position;
        uvs[i]       = vertices[i].uv;
    }

    // ---- 6. 主循环 ----
    while (!window.closed()) {
        window.poll_events();

        buffer.clear(Color(0, 0, 0));

        // ---- 相机：鼠标拖动更新 yaw/pitch ----
        yaw   += window.mouse_dx() * 0.005f;
        pitch -= window.mouse_dy() * 0.005f;

        // pitch 夹紧：上界 π/3 是防止相机翻到地板下面；
        // 下界 -π/24（-7.5°）是因为现在有地板了 —— 相机高度
        // eye_y = r * sin(pitch) = 5 * sin(-7.5°) ≈ -0.65，还在地板（y = -1）之上。
        // 而且这个「贴地」角度正好是透视扭曲最狠的视角，想看清对照实验就往下拖到底。
        pitch = std::clamp(pitch, -kPi / 24.0f, kPi / 3.0f);

        // 球面坐标 → 相机位置，距离原点固定 5。
        // ⚠ r 必须大于「地板离原点最远的角」到原点的距离（≈4.36），见上面第 4 节的说明。
        const float r = 5.0f;
        const float eye_x = r * std::cos(pitch) * std::sin(yaw);
        const float eye_y = r * std::sin(pitch);
        const float eye_z = r * std::cos(pitch) * std::cos(yaw);

        const Mat44 view = Mat44::lookAt(Vec3(eye_x, eye_y, eye_z),
                                         Vec3(0.0f, 0.0f, 0.0f),
                                         Vec3(0.0f, 1.0f, 0.0f));

        const Mat44 model = Mat44::identity();

        // ---- 按键盘切开关（对照实验） ----
        if (window.key_pressed(SDLK_t)) {
            perspective_correct = !perspective_correct;
            std::printf("[T] 透视校正 = %s\n", perspective_correct ? "开" : "关");
        }
        if (window.key_pressed(SDLK_b)) {
            bilinear = !bilinear;
            std::printf("[B] 采样方式 = %s\n", bilinear ? "双线性" : "最近邻");
        }

        // ---- Lambert 光照：对每个顶点算亮度（和第 9 步完全一样） ----
        // 肩上光：光源位置 = 相机 + 右方*4 + 上方*2，跟着相机一起动。
        // 原因见第 9 步：固定世界空间光源在相机绕到背光侧时会让整模型均匀灰。
        const Vec3 eye(eye_x, eye_y, eye_z);
        const Vec3 forward  = normalize(-eye);   // 相机看向原点
        const Vec3 right    = normalize(cross(forward, Vec3(0, 1, 0)));
        const Vec3 cam_up   = cross(right, forward);
        const Vec3 light_pos = eye + right * 4.0f + cam_up * 2.0f;

        // 法线变换：M 是纯旋转（没有非均匀缩放），所以 M 的逆转置 = M。
        for (int i = 0; i < 36; ++i) {
            const Vec3 world_pos    = (model * vertices[i].position).xyz();
            const Vec3 world_normal = model.transformDirection(vertices[i].normal);
            const Vec3 light_dir    = normalize(light_pos - world_pos);
            intensities[i] = lambert_intensity(world_normal, light_dir, 0.2f);
        }

        // 地板的亮度：地板本来就在世界空间，位置直接用，法线恒为 (0,1,0)
        for (int i = 0; i < 4; ++i) {
            const Vec3 light_dir = normalize(light_pos - floor_positions[i]);
            floor_lights[i] = lambert_intensity(floor_normal, light_dir, 0.2f);
        }

        const Mat44 mvp = projection * view * model;

        // ---- 渲染：贴图 × 光照亮度 ----
        // 先画地板再画立方体。其实先后无所谓——深度测试会兜底，
        // 谁在前面谁就留在画面上（第 8 步的成果）。
        renderer::draw_filled_textured(buffer, mvp,
                                       floor_positions, floor_uvs, floor_lights,
                                       4, floor_triangles, 2,
                                       texture,
                                       perspective_correct, bilinear);

        renderer::draw_filled_textured(buffer, mvp,
                                       positions, uvs, intensities,
                                       36, triangles, 12,
                                       texture,
                                       perspective_correct, bilinear);

        window.present(static_cast<const std::uint32_t*>(
            buffer.surface()->pixels));
    }

    return 0;
}