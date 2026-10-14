#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>

#include "math/mat.hpp"
#include "scene/aabb.hpp"
#include "scene/mesh.hpp"

// ============================================================================
//  Scene ── 场景：一堆物体 + 一个相机 + 一个光源
//
//  这一步用到它的两个能力：
//    ① 视锥剔除：拿 view-projection 矩阵算出视锥，逐个物体用包围盒比一下，
//       标出哪些可见，渲染循环只画可见的那些（第 11 步）
//    ② 拿着物体清单：main 不用自己维护「有哪些东西要画」
//
//  SceneObject 里存的是**指针**，不是数据的副本 ——
//  几何数据的本体（Mesh）由加载器产出、main 持有，Scene 只负责
//  「这些几何体各自摆在哪、谁该画」，它不当仓库、也不负责释放。
//
//  第 12 步的改动：几何数据从「裸的位置数组 + 三角形表」换成了 Mesh。
//  好处是 Scene 不再需要若干互相对齐的指针（positions / triangles / count），
//  一个 Mesh 指针就够了 —— 位置、法线、UV、索引、包围盒都在里面。
// ============================================================================

// 场景里的一个物体
struct SceneObject {
    const Mesh* mesh  = nullptr;             // 几何数据（模型空间），Scene 不拥有它
    Mat44       model = Mat44::identity();   // 模型矩阵：把顶点从模型空间摆到世界
    bool        visible = true;              // 上一次 cull() 的结论

    // 第 13 步：这个物体是不是半透明的？
    // 标了 true 的物体：绘制时要开颜色混合、关深度写入，而且**必须排在最后画**。
    bool        transparent = false;
};

class Scene {
public:
    // 加一个物体，返回它的下标
    int add_object(const SceneObject& object);

    int object_count() const { return static_cast<int>(objects_.size()); }

    SceneObject&       object(int index)       { return objects_[index]; }
    const SceneObject& object(int index) const { return objects_[index]; }

    // 用 view-projection（P * V，不含 M）算出视锥，给每个物体打上 visible。
    // 返回值：这一帧可见的物体个数（方便打日志/验证剔除真的生效了）。
    int cull(const Mat44& view_projection);

    int visible_count() const;

    // ---- 第 13 步：算出这一帧的绘制顺序 ----
    //
    // 返回可见物体的下标，按「谁先画」排好：
    //   ① 不透明物体在前（它们之间谁先谁后无所谓 —— 有深度测试兜底）；
    //   ② 半透明物体在后，且**按离相机的距离从远到近**排。
    //
    // 为什么半透明必须从远到近：
    //   混合不可交换。近的盖远的 = 先把远的画上、再拿近的混上去，
    //   等价于「你透过近处玻璃看到远处的东西」，这是对的；
    //   反过来先画近的，远的再来混一次，就变成了「远处的东西盖在近处玻璃前面」，
    //   画面会呈现出错误的前后关系（而且切换视角时穿帮得特别明显）。
    //
    // eye 是相机在世界空间的位置。距离用「世界包围盒中心到眼睛的平方距离」，
    // 不开根号 —— 只比大小，平方就够了。
    std::vector<int> build_draw_order(const Vec3& eye) const;

private:
    std::vector<SceneObject> objects_;
};

#endif
