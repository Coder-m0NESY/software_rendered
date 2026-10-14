#include "scene/frustum.hpp"

#include <cmath>

// ===========================================================================
//  6 个平面是从 view-projection 矩阵里「抠」出来的 —— Gribb-Hartmann 方法
//
//  先看裁剪空间里的 6 个边界条件（NDC 是 [-1,1]³，所以齐次坐标下是 ±w）：
//
//      左： x >= -w  →   x + w >= 0
//      右： x <=  w  →  -x + w >= 0
//      下： y >= -w  →   y + w >= 0
//      上： y <=  w  →  -y + w >= 0
//      近： z >= -w  →   z + w >= 0
//      远： z <=  w  →  -z + w >= 0
//
//  再看裁剪坐标是怎么来的（M 是 view-projection，行主序，乘点在右边）：
//
//      clip.x = row0 · p        （p = (x, y, z, 1)）
//      clip.y = row1 · p
//      clip.z = row2 · p
//      clip.w = row3 · p
//
//  把 clip.x = row0 · p、clip.w = row3 · p 代进「x + w >= 0」：
//
//      row0 · p + row3 · p >= 0   →   (row0 + row3) · p >= 0
//
//  这不就是一个平面方程吗！(row0 + row3) 的前三个分量是法线，第四个分量是 d。
//  于是六个平面就是六次「两行相加/相减」，一行推导都不用背。
//
//  最后把法线归一化，signed_distance 才是真正的「距离」。
// ===========================================================================

namespace {

Plane make_plane(float a, float b, float c, float d)
{
    Plane p;
    const float len = std::sqrt(a * a + b * b + c * c);
    if (len > kEpsilon) {
        const float inv = 1.0f / len;
        p.normal = Vec3(a * inv, b * inv, c * inv);
        p.d      = d * inv;
    } else {
        // 退化平面（理论上到不了）。法线归零时 signed_distance 恒为 0，
        // 会让所有物体都判定为「没被这个面排除掉」—— 保守方向，不会误剔除。
        p.normal = Vec3(0.0f, 0.0f, 0.0f);
        p.d      = 0.0f;
    }
    return p;
}

// row_a + sign * row_b，再归一化成平面
Plane combine(const float* row_a, const float* row_b, float sign)
{
    return make_plane(row_a[0] + sign * row_b[0],
                      row_a[1] + sign * row_b[1],
                      row_a[2] + sign * row_b[2],
                      row_a[3] + sign * row_b[3]);
}

}   // namespace

float Plane::signed_distance(const Vec3& p) const
{
    return normal.dot(p) + d;
}

Frustum Frustum::from_view_projection(const Mat44& view_projection)
{
    const float (*m)[4] = view_projection.m;   // m[行][列]
    const float* r0 = m[0];
    const float* r1 = m[1];
    const float* r2 = m[2];
    const float* r3 = m[3];   // w 那一行

    Frustum f;
    f.planes_[0] = combine(r3, r0,  1.0f);   // 左：  x + w >= 0
    f.planes_[1] = combine(r3, r0, -1.0f);   // 右： -x + w >= 0
    f.planes_[2] = combine(r3, r1,  1.0f);   // 下：  y + w >= 0
    f.planes_[3] = combine(r3, r1, -1.0f);   // 上： -y + w >= 0
    f.planes_[4] = combine(r3, r2,  1.0f);   // 近：  z + w >= 0
    f.planes_[5] = combine(r3, r2, -1.0f);   // 远： -z + w >= 0
    return f;
}

bool Frustum::intersects(const Aabb& box) const
{
    for (int i = 0; i < kPlaneCount; ++i) {
        const Plane& p = planes_[i];

        // 关键一步：取盒子上「沿这个平面法线方向最远」的那个角，
        // 行话叫 positive vertex（p-vertex）。
        // 如果连这个最远的角都在平面外侧，那整个盒子一定都在外侧 → 剔除。
        //
        // 为什么不能取「最近的那个角」：那会把一部分还露在视锥里的盒子误判成在外面，
        // 于是物体该出现的时候不出现 —— 这是最不能犯的错（宁可多画，不能漏画）。
        const Vec3 positive(
            p.normal.x >= 0.0f ? box.max.x : box.min.x,
            p.normal.y >= 0.0f ? box.max.y : box.min.y,
            p.normal.z >= 0.0f ? box.max.z : box.min.z);

        if (p.signed_distance(positive) < 0.0f) {
            return false;   // 完全落在这个平面的外侧
        }
    }
    return true;
}
