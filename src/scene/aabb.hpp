#ifndef AABB_HPP
#define AABB_HPP

#include "math/mat.hpp"
#include "math/vec.hpp"

// ============================================================================
//  Aabb ── 轴对齐包围盒（Axis-Aligned Bounding Box）
//
//  一个跟坐标轴对齐的长方体，用**两个角**就能表示：
//    min = (最小 x, 最小 y, 最小 z)
//    max = (最大 x, 最大 y, 最大 z)
//  一共 6 个数，比存几百个顶点便宜得多。
//
//  为什么要它：视锥体剔除要判断「这个模型在不在相机视野里」。
//  拿模型的每个顶点去比太慢，先用一个盒子把它包起来：
//  盒子整个在视野外 → 模型肯定也在外面 → 整块跳过。
//  这叫「保守判断」：盒子说在外面，一定在外面；
//  盒子说跟视锥有交集，实际可能在也可能不在（宁可多画，不能漏画）。
// ============================================================================
class Aabb {
public:
    Vec3 min;
    Vec3 max;

    Aabb() = default;
    Aabb(const Vec3& min, const Vec3& max);

    // 给一串顶点位置，算出刚好包住它们的最小盒。
    // count <= 0 时返回一个 min > max 的「空盒」，配合 is_valid() 用。
    static Aabb from_positions(const Vec3* positions, int count);

    // min 的每个分量都不大于 max 才算有效
    bool is_valid() const;

    // 用一个矩阵（模型矩阵）把盒子搬到别的空间，重新求一个轴对齐的盒。
    // 注意：轴对齐盒被旋转之后一般**不再是**轴对齐的，所以做法是
    // 把 8 个角都变一遍再重新包一个盒 —— 结果会偏大（保守，不会漏剔除）。
    Aabb transformed(const Mat44& m) const;
};

#endif
