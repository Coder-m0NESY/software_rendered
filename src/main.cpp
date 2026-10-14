// 学习路线第 13 步：Alpha 测试 + 颜色混合，透明物体从远到近排。
//
// 到第 12 步为止，画面里全是「实心」物体：每个通过深度测试的片元都被直接
// 覆盖上去。但现实里的东西不都实心 —— 这步补上两种「不实心」的画法：
//
//   Alpha 测试（alpha test）：alpha 不达标的片元**整个丢掉**，颜色和深度都不写。
//     效果是「镂空」：贴图上 alpha = 0 的地方真的成了洞，能看见后面的东西。
//     场景里那面后墙（栅栏）就是它 —— 按 A 关掉测试，它会糊成一整块实心板。
//
//   颜色混合（color blending）：片元不覆盖，而是跟这个像素已有颜色加权平均：
//       输出 = 源 × α + 目标 × (1 − α)
//     效果是「半透明」：场景里那颗玻璃球看得见背后的圆环，就是它。
//     按 M 关掉混合，玻璃球会变回实心球。
//
// 半透明物体有一条硬性约束：**必须从远到近画**。
// 因为混合不可交换（A 混 B ≠ B 混 A），先画近的、再拿远的往上混，
// 前后关系就反了。所以这一步顺手给 Scene 加了 build_draw_order()：
// 不透明在前、半透明在后（按距离从远到近）—— 这就是所谓「渲染排序」。
//
// 顺便把这堆开关收进了一个 DrawOptions 结构体（见 rasterizer.hpp）：
// 透视校正 / 采样方式 / Alpha 测试 / 颜色混合 / 写深度，一次全带走。
//
// ---- 两种运行方式（第 12 步验收里「能加载任意模型」的补做）----
//   ① 默认（不带参数）：跑上面那套演示场景，用来演示第 13 步的效果。
//   ② 单模型查看：命令行给一个 .obj，或者把 .obj 文件直接拖进窗口 ——
//      程序清空场景、只显示这一个模型。加载后会**自动居中 + 缩放到统一大小**，
//      否则不同来源的模型尺度能差几个数量级（角色 1.4 米、零件 0.001），
//      相机根本对不上焦。
//          SoftRenderer.exe 路径\to\model.obj
//
// 键盘：
//   T  切换透视校正 开/关
//   B  切换采样方式 双线性/最近邻
//   A  切换 Alpha 测试 开/关（看后墙的栅栏变成整块板）
//   M  切换颜色混合 开/关（看玻璃球变回实心）
//   鼠标左键拖动  转相机
//
// 验收：
//   - 后墙能透过栅格洞看见后面的天空/地板；关掉 Alpha 测试后变成实心板
//   - 玻璃球是半透明的，能看见它后面的圆环和球；关掉混合后变实心
//   - 两颗玻璃球前后重叠时，前后关系正确（在 yaw 转到侧面时最明显）
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
#include "io/objloader.hpp"
#include "math/mat.hpp"
#include "math/vec.hpp"
#include "render/lighting.hpp"
#include "render/rasterizer.hpp"   // DrawOptions
#include "render/renderer.hpp"
#include "scene/mesh.hpp"
#include "scene/scene.hpp"

#include <algorithm>   // std::clamp / std::max
#include <cmath>       // std::sin / std::cos
#include <cstdio>
#include <string>      // std::string（命令行参数、拖拽来的路径）
#include <utility>     // std::move
#include <vector>

namespace {

// 渲染一个物体时要交给 renderer 的那点东西。
// 几何数据（Mesh）由加载器产出、main 持有；Scene 只管「谁该画」。
struct RenderItem {
    const Mesh*    mesh       = nullptr;
    const Texture* texture    = nullptr;   // 这个物体贴哪张图
    Mat44          model;                  // 模型矩阵（默认全零，必须显式赋）
    DrawOptions    options;                // 这个物体怎么画（第 13 步）
    int            scene_index = -1;       // 对应 Scene 里的第几个物体
};

// 手搓一个四边形（4 顶点 2 三角形）—— 地板和后墙都用它。
// 默认躺在 XZ 平面上、法线 +Y；要竖起来就用 Mat44::rotateX 把它转 90°。
Mesh make_quad(float half_size, const Vec3& normal)
{
    Mesh mesh;
    mesh.positions = {
        Vec3(-half_size, 0.0f, -half_size), Vec3( half_size, 0.0f, -half_size),
        Vec3( half_size, 0.0f,  half_size), Vec3(-half_size, 0.0f,  half_size),
    };
    mesh.normals = { normal, normal, normal, normal };

    // UV 取「世界坐标 ÷ 2」：一个贴图周期对应 2 个世界单位；
    // 超出 [0,1] 的部分靠 Texture 内部的 wrap 平铺出去。
    const float uv_half = half_size * 0.5f;
    mesh.uvs = {
        Vec2(-uv_half, -uv_half), Vec2( uv_half, -uv_half),
        Vec2( uv_half,  uv_half), Vec2(-uv_half,  uv_half),
    };

    // 绕序：正面朝上（法线 +Y）。第 11 步起填充要求绕序正确，反了整块消失。
    mesh.indices = { 0, 1, 2,  0, 2, 3 };

    mesh.bounds = Aabb::from_positions(mesh.positions.data(), mesh.vertex_count());
    return mesh;
}

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

    Window window("SoftRenderer - alpha blend", kWidth, kHeight);
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

    // ---- 2. 贴图 ----
    // ① 木纹：地板和实心物共用（load 不到就退回代码生成的棋盘格）
    Texture texture = Texture::make_checkerboard(64, 8, Color(240, 226, 200), Color(150, 108, 70));
    if (io::load_texture_bmp("assets/texture_wood.bmp", texture)) {
        std::printf("[贴图] 已加载 assets/texture_wood.bmp（%d×%d）\n",
                    texture.width(), texture.height());
    } else {
        std::printf("[贴图] 加载失败，退回代码生成的棋盘格\n");
    }

    // ② 镂空栅格：整张图只有两种 alpha —— 条上是 255、格子里是 0。
    //    给 Alpha 测试用：alpha = 0 的格子被整块丢弃，板子上就真的出现洞。
    const Texture alpha_tex =
        Texture::make_alpha_grid(128, 32, 8, Color(168, 122, 82));

    // ③ 半透明纯色：整张贴图都是同一个带 alpha 的颜色。
    //    Texture 的构造函数本来就能收带 alpha 的填充色，不用额外写方法。
    //    alpha = 120 大约 47% 不透明 —— 太透明会看不清形状，太不透明又看不出「透」。
    //    两颗球故意用不同颜色：混合不可交换，颜色不同才看得出「谁先谁后」。
    const Texture glass_cyan (8, 8, Color(150, 214, 232, 120));   // 青
    const Texture glass_amber(8, 8, Color(240, 180, 110, 120));   // 琥珀

    // ④ 单模型查看模式的默认贴图：一张灰白纯色。
    //    本项目没解析 .mtl，模型自带的贴图用不上；灰白配 Lambert 光照刚好
    //    能看清形状和棱角（贴彩色反而干扰判断）。
    const Texture gray_tex(8, 8, Color(206, 206, 212));

    // ---- 3. 命令行参数：有参数 = 单模型查看模式 ----
    // argv[1] 是 .obj 路径。带参数就只显示这一个模型；不带就跑演示场景。
    const std::string model_path = (argc > 1) ? std::string(argv[1]) : std::string();
    bool viewer_mode = !model_path.empty();

    // ---- 4. 几何数据 ----
    // 这几个 Mesh 都声明在 main 的作用域里（而不是塞进 if 块里）：
    // items 里存的是**指向它们的指针**，变量一离开作用域就没了，
    // 渲染循环会读到野指针。所以「变量」得一直活着，内容按模式在后面填。
    Mesh torus_mesh;    // 演示场景：圆环
    Mesh sphere_mesh;   // 演示场景：球
    Mesh floor_mesh;    // 演示场景：地板
    Mesh panel_mesh;    // 演示场景：后墙

    // ---- 5. 组装场景 ----
    constexpr float kPi = 3.14159265358979f;

    Scene scene;
    std::vector<RenderItem> items;

    // 往场景里加一个物体：同时登记进 Scene（管剔除 + 排序）和 items（管渲染）
    auto add_object = [&](const Mesh& mesh, const Mat44& model,
                          const Texture& tex, const DrawOptions& options,
                          bool transparent)
    {
        SceneObject obj;
        obj.mesh        = &mesh;
        obj.model       = model;
        obj.transparent = transparent;

        RenderItem item;
        item.scene_index = scene.add_object(obj);
        item.mesh        = &mesh;
        item.texture     = &tex;
        item.model       = model;
        item.options     = options;
        items.push_back(item);
    };

    // 三套「渲染状态」，按物体类型挑一套用。
    // 注意这里只设「这个物体特有的」开关；透视校正 / 采样方式是由键盘全局控制的，
    // 绘制前再统一覆盖进去（反正它们对所有物体都一样）。
    DrawOptions opaque_opts;                    // 实心：写深度、无测试、无混合
    DrawOptions alpha_opts;                     // 镂空：开 Alpha 测试
    alpha_opts.alpha_test      = true;
    alpha_opts.alpha_threshold = 128;
    DrawOptions glass_opts;                     // 玻璃：开混合、不写深度
    glass_opts.blend       = true;
    glass_opts.write_depth = false;

    // ---- 单模型查看模式 ----
    Mesh  viewer_mesh;          // 正在看的那个模型（拖拽换模型时原地覆盖，地址不变）
    float viewer_dist = 3.0f;   // 相机到原点的距离，按模型大小现算

    // 把一个 .obj 装进查看模式：清空场景 + 归一化 + 摆到原点。
    // 抽成 lambda 是因为「拖拽换模型」要再跑一遍完全一样的流程。
    auto load_viewer_model = [&](const std::string& path) -> bool {
        Mesh loaded;
        if (!io::load_obj(path, loaded)) {
            return false;
        }

        // 归一化：居中到原点 + 最长边缩到 2。
        // 有了它，任意尺度的模型都能用同一套相机参数看。
        // （不能省：tao.obj 那种 1.4 米高的角色，直接摆进 r = 5 的相机里只是个小点。）
        loaded.normalize(2.0f);
        viewer_mesh = std::move(loaded);

        // 换模型 = 换场景：旧物体全部丢掉，只留这一个
        scene = Scene();
        items.clear();

        // 相机距离：让归一化后的包围球正好落进视锥、四周留 15% 边距。
        // 球半径取包围盒的**半对角线**（比最长边保守），保证整个盒子都在画面里。
        const Vec3  half     = (viewer_mesh.bounds.max - viewer_mesh.bounds.min) * 0.5f;
        const float radius   = half.length();
        const float half_fov = kPi / 6.0f;                     // fov(60°) 的一半
        viewer_dist = std::max(radius / std::sin(half_fov) * 1.15f, 2.0f);

        add_object(viewer_mesh, Mat44::identity(), gray_tex, opaque_opts, false);
        std::printf("[查看] 单模型查看模式：最长边归一化到 2.0，相机距离 %.2f\n", viewer_dist);
        return true;
    };

    // 命令行给了模型 → 进查看模式；加载失败就退回演示场景
    if (viewer_mode && !load_viewer_model(model_path)) {
        std::printf("[模型] %s 加载失败，改用默认演示场景\n", model_path.c_str());
        viewer_mode = false;
    }

    // ================== 演示场景装配 ==================
    if (!viewer_mode) {
        // 演示用的两个模型 + 两块手搓的四边形，只在演示模式下准备
        if (!io::load_obj("assets/models/torus.obj", torus_mesh)) {
            std::printf("[模型] torus.obj 加载失败，程序退出\n");
            return 1;
        }
        if (!io::load_obj("assets/models/sphere.obj", sphere_mesh)) {
            std::printf("[模型] sphere.obj 加载失败，程序退出\n");
            return 1;
        }
        constexpr float kFloorHalf = 50.0f;
        floor_mesh = make_quad(kFloorHalf, Vec3(0.0f, 1.0f, 0.0f));
        // 后墙用的那块 quad：尺寸小得多，待会儿靠 rotateX 竖起来
        panel_mesh = make_quad(6.0f, Vec3(0.0f, 1.0f, 0.0f));

        // ① 地板（模型矩阵 = 单位阵，顶点已经在世界空间）。
        //    地板在 y = -1，所以先把 quad（y = 0）沉下去一格。
        add_object(floor_mesh, Mat44::translate(0.0f, -1.0f, 0.0f),
                   texture, opaque_opts, false);

        // ② 中央圆环：抬到 y = 1.0 悬在地板上方，放大 1.3 倍，再稍微歪一点
        add_object(torus_mesh,
                   Mat44::translate(0.0f, 1.0f, 0.0f) *
                   Mat44::rotateX(kPi / 6.0f) *
                   Mat44::scale(1.3f, 1.3f, 1.3f),
                   texture, opaque_opts, false);

        // ③ 两圈小球：半径 14 和 26，各 12 个，绕 Y 轴均布，半径 0.6。
        //    半径特意选得比相机距离（r = 5）大不少，这样视野一次只能装下一小段圆弧，
        //    「可见 N / M」这个数字才会明显小于 M —— 否则剔除就看不出效果。
        constexpr int   kRingCount    = 12;
        constexpr float kRingRadius[] = { 14.0f, 26.0f };

        for (float radius : kRingRadius) {
            for (int i = 0; i < kRingCount; ++i) {
                const float angle = (2.0f * kPi / static_cast<float>(kRingCount)) * static_cast<float>(i);
                const Mat44 model = Mat44::translate(radius * std::sin(angle), 0.1f,
                                                     radius * std::cos(angle)) *
                                    Mat44::scale(0.6f, 0.6f, 0.6f);
                add_object(sphere_mesh, model, texture, opaque_opts, false);
            }
        }

        // ④ 后墙：一块竖起来的镂空板子，专门演示 Alpha 测试。
        //    rotateX(π/2) 把「平躺的 quad」立起来：原法线 +Y 变成 +Z，正对相机。
        //    尺寸 12×12（half = 6），放在 z = -12 当背景幕布。
        add_object(panel_mesh,
                   Mat44::translate(0.0f, 3.0f, -12.0f) * Mat44::rotateX(kPi / 2.0f),
                   alpha_tex, alpha_opts, false);

        // ⑤ 两颗半透明玻璃球：一近一远、部分重叠，专门演示颜色混合 + 绘制排序。
        //    位置挑得有点讲究 —— 相机在 r = 5 的球面上转，太靠前（z 大）的球会贴脸，
        //    屏幕上大得离谱；所以两颗都放在相机前方 4.5 ~ 7 个单位的距离上，
        //    并偏在右侧 x ≈ 1.7 ~ 2.6：
        //      · 近的那颗（青）压住圆环右半边 —— 能透过它看见圆环，混合生效的直接证据；
        //      · 远的那颗（琥珀）跟近的有交叠 —— 排序对不对，看交叠处谁盖谁就知道。
        //    两球颜色故意不同：颜色一样的话先后顺序对结果没影响，就演示不出排序了。
        const Vec3  kGlassPos[] = { Vec3(1.7f, 1.05f,  0.5f),   // 近：青
                                    Vec3(2.6f, 1.05f, -2.2f) }; // 远：琥珀
        const Texture* kGlassTex[] = { &glass_cyan, &glass_amber };
        for (int i = 0; i < 2; ++i) {
            add_object(sphere_mesh,
                       Mat44::translate(kGlassPos[i].x, kGlassPos[i].y, kGlassPos[i].z) *
                       Mat44::scale(0.8f, 0.8f, 0.8f),
                       *kGlassTex[i], glass_opts, true);
        }

        std::printf("[场景] 共 %d 个物体（1 地板 + 1 圆环 + %d 小球 + 1 镂空墙 + 2 玻璃球）\n",
                    scene.object_count(), kRingCount * 2);
    }

    // ---- 6. 投影 + 相机状态 ----
    const Mat44 projection = Mat44::perspective(kPi / 3.0f,
                                                static_cast<float>(kWidth) /
                                                    static_cast<float>(kHeight),
                                                0.1f, 100.0f);

    float yaw   = 0.0f;     // 绕 Y 轴：0 时相机在 +Z，朝 -Z 看
    float pitch = 0.0f;     // 绕 X 轴

    // 环境光下限（0~1）：背光面保底亮度。
    constexpr float kAmbient = 0.35f;

    // 背景天空渐变：上（天顶）到下（地平线）。
    constexpr int kSkyBands      = 75;                        // 75 条 × 8 像素 = 600
    constexpr int kSkyBandHeight = kHeight / kSkyBands;
    const Color   kSkyTop(44, 58, 84);
    const Color   kSkyHorizon(132, 150, 172);

    // 对照实验开关
    bool perspective_correct = true;    // T
    bool bilinear            = false;   // B
    bool alpha_test_on       = true;    // A
    bool blend_on            = true;    // M

    // 上一次打印过的「可见数」，只在变化时打日志，免得刷屏
    int last_visible = -1;

    // ---- 7. 主循环 ----
    while (!window.closed()) {
        window.poll_events();

        // ---- 把一个 .obj 拖进窗口 → 换成它（顺手切到查看模式）----
        // 拖拽是「事件」，所以必须在 poll_events() 之后、拿这一帧的状态判断。
        if (!window.drop_file().empty()) {
            const std::string& dropped = window.drop_file();
            if (load_viewer_model(dropped)) {
                viewer_mode = true;
                yaw   = 0.0f;   // 换了模型，视角回到正面
                pitch = 0.0f;
            }
        }

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

        // pitch 夹紧：
        //   演示模式：上界 π/3 防止相机摆得太平；下界 -π/24（-7.5°）保证
        //             eye_y = 5 * sin(-7.5°) ≈ -0.65，还在地板（y = -1）之上。
        //   查看模式：没有地板，可以上下绕着模型看一整圈。上限卡在 ±0.48π，
        //             是为了避开「视线与 up 向量 (0,1,0) 平行」—— 那样 lookAt 会退化。
        if (viewer_mode) {
            pitch = std::clamp(pitch, -kPi * 0.48f, kPi * 0.48f);
        } else {
            pitch = std::clamp(pitch, -kPi / 24.0f, kPi / 3.0f);
        }

        // 球面坐标 → 相机位置。演示场景固定 r = 5；查看模式按模型大小现算
        const float r = viewer_mode ? viewer_dist : 5.0f;
        const float eye_x = r * std::cos(pitch) * std::sin(yaw);
        const float eye_y = r * std::sin(pitch);
        const float eye_z = r * std::cos(pitch) * std::cos(yaw);
        const Vec3  eye(eye_x, eye_y, eye_z);

        const Mat44 view = Mat44::lookAt(eye,
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
        if (window.key_pressed(SDLK_a)) {
            alpha_test_on = !alpha_test_on;
            std::printf("[A] Alpha 测试 = %s\n", alpha_test_on ? "开（镂空）" : "关（实心板）");
        }
        if (window.key_pressed(SDLK_m)) {
            blend_on = !blend_on;
            std::printf("[M] 颜色混合 = %s\n", blend_on ? "开（半透明）" : "关（实心球）");
        }

        // ---- ① 视锥剔除：整帧只判一次，给每个物体打上 visible ----
        const int visible = scene.cull(view_projection);
        if (visible != last_visible) {
            std::printf("[剔除] 可见 %d / %d\n", visible, scene.object_count());
            last_visible = visible;
        }

        // ---- ② 绘制顺序：不透明在前，半透明按距离从远到近排（第 13 步）----
        // 剔除已经打过 visible，这里只挑可见的，并把半透明排到后面、由远及近。
        const std::vector<int> draw_order = scene.build_draw_order(eye);

        // ---- 肩上光：光源位置 = 相机 + 右方*4 + 上方*2，跟着相机一起动 ----
        // （和第 9、10 步一样：固定世界空间光源会在绕到背光侧时让整模型一片灰）
        const Vec3 forward = normalize(-eye);                       // 相机看向原点
        const Vec3 right   = normalize(cross(forward, Vec3(0, 1, 0)));
        const Vec3 cam_up  = cross(right, forward);
        const Vec3 light_pos = eye + right * 4.0f + cam_up * 2.0f;

        // ---- ③ 按顺序逐个物体：算光照 + 送进 renderer ----
        for (int index : draw_order) {
            const RenderItem& item = items[index];
            const Mesh& mesh = *item.mesh;
            const int vertex_count = mesh.vertex_count();

            // 顶点亮度（Lambert）：法线要跟着 model 一起变。
            // model 若带旋转/缩放，法线的变换不能直接乘 model（要用逆转置），
            // 这里的 model 只有平移和等比缩放，走 transformDirection 结果是对的。
            std::vector<float> light(static_cast<std::size_t>(vertex_count));
            for (int i = 0; i < vertex_count; ++i) {
                const Vec3 world_pos    = (item.model * mesh.positions[i]).xyz();
                const Vec3 world_normal = item.model.transformDirection(mesh.normals[i]);
                const Vec3 light_dir    = normalize(light_pos - world_pos);
                light[i] = lambert_intensity(world_normal, light_dir, kAmbient);
            }

            // MVP = P * V * M，比别的物体多一个 M
            const Mat44 mvp = view_projection * item.model;

            // 把「全局开关」覆盖进这个物体的状态里：
            // 透视校正 / 采样方式是全场统一的；A、M 两个键是「临时关掉某个效果」，
            // 用来跟开着的时候对比。
            DrawOptions opts = item.options;
            opts.perspective_correct = perspective_correct;
            opts.bilinear            = bilinear;
            if (!alpha_test_on) {
                opts.alpha_test = false;
            }
            if (!blend_on && opts.blend) {
                // 关掉混合的玻璃球要当实心画：不写深度的话它会跟后面的东西乱插，
                // 所以这里顺手把写深度也打开。
                opts.blend       = false;
                opts.write_depth = true;
            }

            renderer::draw_filled_textured(buffer, mvp,
                                           mesh.positions.data(), mesh.uvs.data(), light.data(),
                                           vertex_count,
                                           mesh.indices.data(), mesh.triangle_count(),
                                           *item.texture,
                                           opts);
        }

        window.present(static_cast<const std::uint32_t*>(buffer.surface()->pixels));
    }

    return 0;
}
