// 学习路线第 11 步：背面剔除 + 视锥剔除 + 近平面裁剪。
//
// 在第 10 步（纹理 + 透视校正）的基础上加了三件事：
//   ① 背面剔除   —— 屏幕空间有向面积 <= 0 的三角形，在填像素前整块丢掉
//                    （render/rasterizer.cpp，最省事的一个）
//   ② 近平面裁剪 —— Sutherland-Hodgman 把跨过近平面的三角形切开
//                    （render/clipper.cpp，最难的一个）
//   ③ 视锥剔除   —— 每个物体算个包围盒，跟视锥六个面比一下，全在外面就跳过
//                    （scene/aabb + scene/frustum + scene/scene，CPU 侧的事）
//
// 场景改成「Scene 驱动」：一块大地板 + 中央立方体 + 两圈小立方体。
// 后两样是为了让「视锥剔除」看得见——小立方体放在半径 14 / 26 的圆上，
// 转动相机时控制台会打印「可见 N / M」，数字跟着视野变。
//
// 另一个附带的好处：第 10 步因为「整块丢弃」而不敢铺大的地板，
// 现在裁剪做完，可以一路铺到地平线，转相机也不会缺块。
//
// 键盘：
//   T  切换透视校正 开/关  —— 关掉看一眼真实的扭曲
//   B  切换采样方式 双线性/最近邻
//   鼠标左键拖动  转相机
//
// 验收：
//   - 地板铺到地平线，怎么转都不缺块（裁剪）
//   - 相机贴到立方体上，画面正常被切，没有横穿全屏的三角形飞出来（裁剪）
//   - 转动相机，「可见 N / M」跟着变（视锥剔除）
//   - 立方体外观和第 10 步一致（背面剔除不该让画面变样，它只省算力）
#include <SDL.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "core/color.hpp"
#include "core/pixelbuffer.hpp"
#include "core/texture.hpp"
#include "core/window.hpp"
#include "io/image.hpp"
#include "math/mat.hpp"
#include "math/vec.hpp"
#include "render/lighting.hpp"
#include "render/renderer.hpp"
#include "scene/aabb.hpp"
#include "scene/scene.hpp"
#include "scene/vertex.hpp"

#include <algorithm>   // std::clamp
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

// 渲染一个物体时要交给 renderer 的那点东西。
// Scene 只管「谁该画」，顶点属性（UV、亮度）还是 main 自己拿着。
struct RenderItem {
    int          scene_index = -1;             // 对应 Scene 里的第几个物体
    const Vec3*  positions   = nullptr;
    const Vec3*  normals     = nullptr;
    const Vec2*  uvs         = nullptr;
    int          vertex_count = 0;
    const int  (*triangles)[3] = nullptr;
    int          triangle_count = 0;
    Mat44        model;                        // 模型矩阵（默认全零，必须显式赋）
};

}   // namespace

int main(int argc, char* argv[])
{
#if defined(_WIN32)
    // Windows 控制台默认是 GBK(936) 代码页，而本文件里的字符串是 UTF-8，
    // 直接 printf 中文就会显示成乱码。把控制台输入/输出都切成 UTF-8 即可。
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // 本程序会一直渲染、正常退出前不返回，stdout 又是全缓冲，
    // 日志会攒在缓冲区里不显示。这里改成不缓冲，printf 立刻可见。
    std::setvbuf(stdout, nullptr, _IONBF, 0);
#endif

    // ---- 1. 窗口和画布 ----
    constexpr int kWidth  = 800;
    constexpr int kHeight = 600;

    Window window("SoftRenderer - culling & clipping", kWidth, kHeight);
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

    // ---- 2. 贴图：优先读一张真实图片，读不到就退回代码生成的棋盘格 ----
    Texture texture = Texture::make_checkerboard(64, 8, Color(240, 226, 200), Color(150, 108, 70));
    if (io::load_texture_bmp("assets/texture_wood.bmp", texture)) {
        std::printf("[贴图] 已加载 assets/texture_wood.bmp（%d×%d）\n",
                    texture.width(), texture.height());
    } else {
        std::printf("[贴图] 加载失败，退回代码生成的棋盘格\n");
    }

    // ---- 3. 立方体网格：36 顶点，每面 6 个（2 三角形 × 3 顶点），带独立法线、UV ----
    // 绕序约定：从立方体外面看是顺时针（CW）——第 11 步的背面剔除**依赖**这个约定。
    // （屏幕 y 轴向下，正面在屏幕空间的有向面积 > 0，恰好对应「从外面看顺时针」。）
    const Vertex cube_vertices[36] = {
        // 远面 z=-1，法线 (0,0,-1)
        { Vec3(-1,-1,-1), Vec3(0,0,-1), Vec2(0,1) }, { Vec3( 1,-1,-1), Vec3(0,0,-1), Vec2(1,1) }, { Vec3( 1, 1,-1), Vec3(0,0,-1), Vec2(1,0) },
        { Vec3(-1,-1,-1), Vec3(0,0,-1), Vec2(0,1) }, { Vec3( 1, 1,-1), Vec3(0,0,-1), Vec2(1,0) }, { Vec3(-1, 1,-1), Vec3(0,0,-1), Vec2(0,0) },
        // 近面 z=+1，法线 (0,0,+1)
        { Vec3(-1,-1, 1), Vec3(0,0, 1), Vec2(0,1) }, { Vec3(-1, 1, 1), Vec3(0,0, 1), Vec2(0,0) }, { Vec3( 1, 1, 1), Vec3(0,0, 1), Vec2(1,0) },
        { Vec3(-1,-1, 1), Vec3(0,0, 1), Vec2(0,1) }, { Vec3( 1, 1, 1), Vec3(0,0, 1), Vec2(1,0) }, { Vec3( 1,-1, 1), Vec3(0,0, 1), Vec2(1,1) },
        // 底面 y=-1，法线 (0,-1,0)
        { Vec3(-1,-1,-1), Vec3(0,-1,0), Vec2(0,0) }, { Vec3(-1,-1, 1), Vec3(0,-1,0), Vec2(0,1) }, { Vec3( 1,-1, 1), Vec3(0,-1,0), Vec2(1,1) },
        { Vec3(-1,-1,-1), Vec3(0,-1,0), Vec2(0,0) }, { Vec3( 1,-1, 1), Vec3(0,-1,0), Vec2(1,1) }, { Vec3( 1,-1,-1), Vec3(0,-1,0), Vec2(1,0) },
        // 顶面 y=+1，法线 (0,+1,0)
        { Vec3(-1, 1,-1), Vec3(0, 1,0), Vec2(0,0) }, { Vec3( 1, 1, 1), Vec3(0, 1,0), Vec2(1,1) }, { Vec3(-1, 1, 1), Vec3(0, 1,0), Vec2(0,1) },
        { Vec3(-1, 1,-1), Vec3(0, 1,0), Vec2(0,0) }, { Vec3( 1, 1,-1), Vec3(0, 1,0), Vec2(1,0) }, { Vec3( 1, 1, 1), Vec3(0, 1,0), Vec2(1,1) },
        // 左面 x=-1，法线 (-1,0,0)
        // ⚠ 第 11 步修正：从外面看这面的缠绕方向原本和其余五面相反，背面剔除一开
        //    就会把左面当成正面留下（贴脸时正面被近平面切掉，它就露出来了）。
        //    对调每条三角形的后两个顶点即可把绕序摆正。
        { Vec3(-1,-1,-1), Vec3(-1,0,0), Vec2(0,1) }, { Vec3(-1, 1, 1), Vec3(-1,0,0), Vec2(1,0) }, { Vec3(-1,-1, 1), Vec3(-1,0,0), Vec2(1,1) },
        { Vec3(-1,-1,-1), Vec3(-1,0,0), Vec2(0,1) }, { Vec3(-1, 1,-1), Vec3(-1,0,0), Vec2(0,0) }, { Vec3(-1, 1, 1), Vec3(-1,0,0), Vec2(1,0) },
        // 右面 x=+1，法线 (+1,0,0)（同样在第 11 步修正了绕序）
        { Vec3( 1,-1,-1), Vec3( 1,0,0), Vec2(0,1) }, { Vec3( 1,-1, 1), Vec3( 1,0,0), Vec2(1,1) }, { Vec3( 1, 1, 1), Vec3( 1,0,0), Vec2(1,0) },
        { Vec3( 1,-1,-1), Vec3( 1,0,0), Vec2(0,1) }, { Vec3( 1, 1, 1), Vec3( 1,0,0), Vec2(1,0) }, { Vec3( 1, 1,-1), Vec3( 1,0,0), Vec2(0,0) },
    };

    const int cube_triangles[12][3] = {
        { 0, 1, 2}, { 3, 4, 5},   // 远面
        { 6, 7, 8}, { 9,10,11},   // 近面
        {12,13,14}, {15,16,17},   // 底面
        {18,19,20}, {21,22,23},   // 顶面
        {24,25,26}, {27,28,29},   // 左面
        {30,31,32}, {33,34,35},   // 右面
    };

    // renderer 要的是「一串连续的 Vec3 / Vec2」，而 Vertex 是结构体数组，
    // 所以位置、法线、UV 各拷一份出来（初始化一次就够）。
    constexpr int kCubeVertexCount = 36;
    Vec3 cube_positions[kCubeVertexCount];
    Vec3 cube_normals[kCubeVertexCount];
    Vec2 cube_uvs[kCubeVertexCount];
    for (int i = 0; i < kCubeVertexCount; ++i) {
        cube_positions[i] = cube_vertices[i].position;
        cube_normals[i]   = cube_vertices[i].normal;
        cube_uvs[i]       = cube_vertices[i].uv;
    }

    // ---- 4. 地板：一块大地板（4 顶点 2 三角形），从近处一路铺到地平线 ----
    //
    // ⚠ 第 10 步这块只有 6×6，注释里写着「不能铺大，否则有顶点跑到相机背后，
    //    renderer 会把整块三角形丢掉，地板缺一块」。第 11 步裁剪到位之后，
    //    这个约束**解除了** —— 直接铺 100×100。
    //
    // UV 取「世界坐标 ÷ 2」：一个贴图周期对应 2 个世界单位，格子尺寸和
    // 立方体一个面一致；超出 [0,1] 的部分靠 Texture 内部的 wrap 平铺出去。
    constexpr float kFloorHalf = 50.0f;
    const Vec3 floor_positions[4] = {
        Vec3(-kFloorHalf, -1.0f, -kFloorHalf), Vec3( kFloorHalf, -1.0f, -kFloorHalf),
        Vec3( kFloorHalf, -1.0f,  kFloorHalf), Vec3(-kFloorHalf, -1.0f,  kFloorHalf)
    };
    const Vec3 floor_normals[4] = {
        Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f),
        Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)
    };
    const Vec2 floor_uvs[4] = {
        Vec2(-kFloorHalf * 0.5f, -kFloorHalf * 0.5f), Vec2( kFloorHalf * 0.5f, -kFloorHalf * 0.5f),
        Vec2( kFloorHalf * 0.5f,  kFloorHalf * 0.5f), Vec2(-kFloorHalf * 0.5f,  kFloorHalf * 0.5f)
    };
    const int floor_triangles[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } };

    // ---- 5. 组装场景：地板 + 中央立方体 + 两圈小立方体 ----
    constexpr float kPi = 3.14159265358979f;

    Scene scene;
    std::vector<RenderItem> items;

    // 往场景里加一个物体：同时登记进 Scene（管剔除）和 items（管渲染）
    auto add_object = [&](const Vec3* positions, const Vec3* normals, const Vec2* uvs,
                          int vertex_count,
                          const int (*triangles)[3], int triangle_count,
                          const Mat44& model)
    {
        SceneObject obj;
        obj.positions      = positions;
        obj.position_count = vertex_count;
        obj.triangles      = triangles;
        obj.triangle_count = triangle_count;
        obj.model          = model;
        obj.bounds         = Aabb::from_positions(positions, vertex_count);   // 模型空间包围盒

        RenderItem item;
        item.scene_index    = scene.add_object(obj);
        item.positions      = positions;
        item.normals        = normals;
        item.uvs            = uvs;
        item.vertex_count   = vertex_count;
        item.triangles      = triangles;
        item.triangle_count = triangle_count;
        item.model          = model;
        items.push_back(item);
    };

    // ① 地板（模型矩阵 = 单位阵，顶点已经在世界空间）
    add_object(floor_positions, floor_normals, floor_uvs, 4,
               floor_triangles, 2, Mat44::identity());

    // ② 中央立方体
    add_object(cube_positions, cube_normals, cube_uvs, kCubeVertexCount,
               cube_triangles, 12, Mat44::identity());

    // ③ 两圈小立方体：半径 14 和 26，各 12 个，绕 Y 轴均布。
    //    半径特意选得比相机距离（r = 5）大不少，这样视野一次只能装下一小段圆弧，
    //    「可见 N / M」这个数字才会明显小于 M —— 否则剔除就看不出效果。
    constexpr int   kRingCount    = 12;
    constexpr float kRingRadius[] = { 14.0f, 26.0f };

    for (float radius : kRingRadius) {
        for (int i = 0; i < kRingCount; ++i) {
            const float angle = (2.0f * kPi / static_cast<float>(kRingCount)) * static_cast<float>(i);
            const Mat44 model = Mat44::translate(radius * std::sin(angle), 0.0f,
                                                 radius * std::cos(angle));
            add_object(cube_positions, cube_normals, cube_uvs, kCubeVertexCount,
                       cube_triangles, 12, model);
        }
    }

    std::printf("[场景] 共 %d 个物体（1 地板 + 1 中央立方体 + %d 小立方体）\n",
                scene.object_count(), kRingCount * 2);

    // ---- 6. 投影 + 相机状态 ----
    const Mat44 projection = Mat44::perspective(kPi / 3.0f,
                                                static_cast<float>(kWidth) /
                                                    static_cast<float>(kHeight),
                                                0.1f, 100.0f);

    float yaw   = 0.0f;     // 绕 Y 轴：0 时相机在 +Z，朝 -Z 看
    float pitch = 0.0f;     // 绕 X 轴

    // 环境光下限（0~1）：背光面保底亮度。0.2 时背光面几乎全黑，整个画面偏暗；
    // 提到 0.35 让阴影浅一些，又不至于把明暗关系压没。
    constexpr float kAmbient = 0.35f;

    // 背景天空渐变：上（天顶）到下（地平线）。纯黑背景会让画面发闷。
    constexpr int kSkyBands      = 75;                        // 75 条 × 8 像素 = 600
    constexpr int kSkyBandHeight = kHeight / kSkyBands;
    const Color   kSkyTop(44, 58, 84);
    const Color   kSkyHorizon(132, 150, 172);

    // 对照实验开关：T 切透视校正，B 切采样方式
    bool perspective_correct = true;
    bool bilinear            = false;

    // 上一次打印过的「可见数」，只在变化时打日志，免得刷屏
    int last_visible = -1;

    // ---- 7. 主循环 ----
    while (!window.closed()) {
        window.poll_events();

        // 背景：只清深度（颜色交给下面的渐变天空铺），
        // 逐条画横带拼出一个从深蓝到灰蓝的渐变。
        buffer.clear_depth();
        for (int i = 0; i < kSkyBands; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(kSkyBands - 1);
            auto mix = [t](uint8_t a, uint8_t b) -> uint8_t {
                return static_cast<uint8_t>(static_cast<float>(a) +
                                            (static_cast<float>(b) - a) * t);
            };
            buffer.draw_filled_rect(0, i * kSkyBandHeight, kWidth, kSkyBandHeight,
                                    Color(mix(kSkyTop.r, kSkyHorizon.r),
                                          mix(kSkyTop.g, kSkyHorizon.g),
                                          mix(kSkyTop.b, kSkyHorizon.b)));
        }

        // ---- 相机：鼠标拖动更新 yaw/pitch ----
        yaw   += window.mouse_dx() * 0.005f;
        pitch -= window.mouse_dy() * 0.005f;

        // pitch 夹紧：上界 π/3 防止相机摆得太平；下界 -π/24（-7.5°）保证
        // eye_y = r * sin(pitch) = 5 * sin(-7.5°) ≈ -0.65，还在地板（y = -1）之上。
        pitch = std::clamp(pitch, -kPi / 24.0f, kPi / 3.0f);

        // 球面坐标 → 相机位置，距离原点固定 5
        const float r = 5.0f;
        const float eye_x = r * std::cos(pitch) * std::sin(yaw);
        const float eye_y = r * std::sin(pitch);
        const float eye_z = r * std::cos(pitch) * std::cos(yaw);

        const Mat44 view = Mat44::lookAt(Vec3(eye_x, eye_y, eye_z),
                                         Vec3(0.0f, 0.0f, 0.0f),
                                         Vec3(0.0f, 1.0f, 0.0f));

        // 视锥只跟相机有关，所以剔除用的是 P * V（**不含** M）
        const Mat44 view_projection = projection * view;

        // ---- 按键盘切开关（对照实验） ----
        if (window.key_pressed(SDLK_t)) {
            perspective_correct = !perspective_correct;
            std::printf("[T] 透视校正 = %s\n", perspective_correct ? "开" : "关");
        }
        if (window.key_pressed(SDLK_b)) {
            bilinear = !bilinear;
            std::printf("[B] 采样方式 = %s\n", bilinear ? "双线性" : "最近邻");
        }

        // ---- ① 视锥剔除：整帧只判一次，给每个物体打上 visible ----
        const int visible = scene.cull(view_projection);
        if (visible != last_visible) {
            std::printf("[剔除] 可见 %d / %d\n", visible, scene.object_count());
            last_visible = visible;
        }

        // ---- 肩上光：光源位置 = 相机 + 右方*4 + 上方*2，跟着相机一起动 ----
        // （和第 9、10 步一样：固定世界空间光源会在绕到背光侧时让整模型一片灰）
        const Vec3 eye(eye_x, eye_y, eye_z);
        const Vec3 forward = normalize(-eye);                       // 相机看向原点
        const Vec3 right   = normalize(cross(forward, Vec3(0, 1, 0)));
        const Vec3 cam_up  = cross(right, forward);
        const Vec3 light_pos = eye + right * 4.0f + cam_up * 2.0f;

        // ---- ② 逐个物体：不可见的直接跳过，可见的算光照 + 送进 renderer ----
        for (const RenderItem& item : items) {
            if (!scene.object(item.scene_index).visible) {
                continue;   // 视锥剔除生效：这一整个物体的所有三角形都不用算
            }

            // 顶点亮度（Lambert）：法线要跟着 model 一起变。
            // model 是纯平移，transformDirection 对方向无影响，但仍然走这条路
            // —— 以后 model 带旋转、缩放时不用再改这里。
            float light[kCubeVertexCount];
            for (int i = 0; i < item.vertex_count; ++i) {
                const Vec3 world_pos    = (item.model * item.positions[i]).xyz();
                const Vec3 world_normal = item.model.transformDirection(item.normals[i]);
                const Vec3 light_dir    = normalize(light_pos - world_pos);
                light[i] = lambert_intensity(world_normal, light_dir, kAmbient);
            }

            // MVP = P * V * M，比别的物体多一个 M
            const Mat44 mvp = view_projection * item.model;

            renderer::draw_filled_textured(buffer, mvp,
                                           item.positions, item.uvs, light,
                                           item.vertex_count,
                                           item.triangles, item.triangle_count,
                                           texture,
                                           perspective_correct, bilinear);
        }

        window.present(static_cast<const std::uint32_t*>(buffer.surface()->pixels));
    }

    return 0;
}
