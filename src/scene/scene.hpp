#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>

#include "math/mat.hpp"
#include "scene/aabb.hpp"

// ============================================================================
//  Scene ── 场景：一堆物体 + 一个相机 + 一个光源
//
//  这一步只用到它的「剔除」能力：
//    ① 拿 view-projection 矩阵算出视锥
//    ② 逐个物体，用包围盒跟视锥比一下，标出哪些可见
//    ③ 渲染循环只画可见的那些
//
//  SceneObject 里存的是**指针**，不是数据的副本 ——
//  顶点数组还在 main 里，Scene 只负责「判断谁该画」，不当仓库。
//  相机和光源要等第 12 步引进来，这一版先只管剔除。
// ============================================================================

// 场景里的一个物体
struct SceneObject {
    const Vec3* positions      = nullptr;   // 顶点位置（模型空间）
    int         position_count = 0;

    const int (*triangles)[3]  = nullptr;   // 三角形表（每行三个顶点下标）
    int         triangle_count = 0;

    Mat44 model = Mat44::identity();        // 模型矩阵：把顶点从模型空间摆到世界
    Aabb  bounds;                           // 模型空间的包围盒（由 positions 算出来）
    bool  visible = true;                   // 上一次 cull() 的结论
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

private:
    std::vector<SceneObject> objects_;
};

#endif
