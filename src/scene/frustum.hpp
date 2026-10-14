#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include "math/mat.hpp"
#include "math/vec.hpp"
#include "scene/aabb.hpp"

// ============================================================================
//  Frustum ── 视锥体：相机能看到的那块空间
//
//  它是个四棱锥台（从相机往前截一段），被 6 个平面围出来：
//  左、右、下、上、近、远。
//
//  判断「物体在不在视野里」= 判断「包围盒是不是完全落在这 6 个平面之外」。
//
//  平面表示法：法线 + 常数，dot(normal, p) + d = 0。
//  法线归一化之后，dot(normal, p) + d 就是 p 到平面的**带符号距离**：
//    > 0 ：在平面内侧（视锥里面那一侧）
//    < 0 ：在平面外侧
//
//  6 个平面怎么来：不用手推，直接从 view-projection 矩阵里「抠」出来
//  （Gribb-Hartmann 方法，1998）。原理见 frustum.cpp。
// ============================================================================

struct Plane {
    Vec3  normal;       // 单位向量，指向视锥**内部**
    float d = 0.0f;     // dot(normal, p) + d >= 0 表示 p 在内侧

    float signed_distance(const Vec3& p) const;
};

class Frustum {
public:
    static constexpr int kPlaneCount = 6;

    // 从 view-projection 矩阵（P * V，**不含** M）提取 6 个平面。
    // 之所以不含 M：视锥是相机属性，跟具体物体摆在哪儿无关。
    static Frustum from_view_projection(const Mat44& view_projection);

    // 盒子跟视锥有交集 → true。
    // 保守：返回 true 不代表一定看得见（可能只是擦边），
    // 但返回 false 一定看不见 —— 不会把该画的物体剔掉。
    bool intersects(const Aabb& box) const;

    const Plane& plane(int i) const { return planes_[i]; }

private:
    Plane planes_[kPlaneCount];
};

#endif
