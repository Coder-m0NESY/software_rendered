#include "clipper.hpp"

// ===========================================================================
//  Sutherland-Hodgman：拿一个平面去切一个多边形
//
//  一句话版本 —— 沿着多边形的边走一圈，对每条边 (cur → nxt) 问两件事：
//    ① cur 在平面内侧吗？在就把它留下来。
//    ② 这条边跨过平面了吗？跨了就补一个交点。
//  走完一圈，留下的点连起来就是被切完的多边形。
//
//  对三角形来说，切一刀的结果只可能是：空（全在外）、三角形（全在内）、
//  或者四边形（一个角被切掉）。所以临时数组开 4 个位置就够。
// ===========================================================================

namespace {

// 平面函数：>= 0 在近平面内侧（保留），< 0 在外侧（丢掉）
float plane_distance(const ClipVertex& v)
{
    return v.position.z + v.position.w;
}

// 在两个顶点之间按 t 插出新的裁剪顶点。
// 位置、UV、亮度全都跟着**同一个 t** 线性插 —— 这就是上面说的「天生透视正确」。
ClipVertex lerp_vertex(const ClipVertex& a, const ClipVertex& b, float t)
{
    ClipVertex r;
    r.position = lerp(a.position, b.position, t);
    r.uv       = lerp(a.uv, b.uv, t);
    r.light    = a.light + (b.light - a.light) * t;
    return r;
}

}   // namespace

int clipper::clip_near_plane(const ClipVertex in[3], ClipVertex out[kMaxOutputTriangles * 3])
{
    // ---- 第一步：切一刀，得到 0 ~ 4 个顶点的多边形 ----
    ClipVertex poly[4];
    int count = 0;

    for (int i = 0; i < 3; ++i) {
        const ClipVertex& cur = in[i];
        const ClipVertex& nxt = in[(i + 1) % 3];

        const float d_cur = plane_distance(cur);
        const float d_nxt = plane_distance(nxt);
        const bool  cur_in = (d_cur >= 0.0f);
        const bool  nxt_in = (d_nxt >= 0.0f);

        // ① 当前顶点在内侧 → 留下
        if (cur_in) {
            poly[count++] = cur;
        }
        // ② 这条边的两端一内一外 → 补一个交点
        //    交点比例：d(t) = d_cur + t * (d_nxt - d_cur) = 0  →  t = d_cur / (d_cur - d_nxt)
        if (cur_in != nxt_in) {
            const float t = d_cur / (d_cur - d_nxt);
            poly[count++] = lerp_vertex(cur, nxt, t);
        }
    }

    // ---- 第二步：把多边形扇成三角形 ----
    // 拿 poly[0] 当扇心，和后面每两个相邻点组成一个三角形：
    //   0 个点 → 没有输出；3 个点 → 1 个三角形；4 个点 → 2 个三角形。
    // 切分不改变绕序，所以切完照样能做背面剔除。
    int triangles = 0;
    for (int i = 1; i + 1 < count; ++i) {
        out[triangles * 3 + 0] = poly[0];
        out[triangles * 3 + 1] = poly[i];
        out[triangles * 3 + 2] = poly[i + 1];
        ++triangles;
    }
    return triangles;
}
