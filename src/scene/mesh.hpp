#ifndef MESH_HPP
#define MESH_HPP

#include <algorithm>   // std::max
#include <vector>

#include "math/vec.hpp"
#include "scene/aabb.hpp"

// ============================================================================
//  Mesh ── 一份「顶点数组 + 索引数组」的几何数据
//
//  第 12 步之前，模型的顶点和三角形表都直接写在 main.cpp 里（手写立方体）：
//    const Vec3 cube_positions[36] = { ... };
//    const int  cube_triangles[12][3] = { ... };
//  那是「一个模型一段硬编码」，想换模型就得重抄一遍。
//
//  这里把这件事抽出来：一个模型 = 一个 Mesh。
//  加载器（io/objloader）负责产出 Mesh，main 只负责拿来摆进场景。
//
//  为什么要**索引**（indices）而不是每个三角形直接写三个顶点（顶点展开）：
//    一个 800 顶点的圆环展开后要写 1600×3 = 4800 个顶点，其中大量重复。
//    用索引则顶点只存一份，三角形只存下标 —— 省内存，而且顶点着色只算一次。
//    代价是要多一次「按索引取顶点」的间接寻址，以及顶点必须去重。
//
//  这就是渲染管线上「图元装配（Primitive Assembly）」那一步要的数据形态。
// ============================================================================
struct Mesh {
    std::vector<Vec3> positions;   // 模型空间的位置
    std::vector<Vec3> normals;     // 模型空间的法线（单位向量）
    std::vector<Vec2> uvs;         // 贴图坐标，(0,0) 在左上角、v 向下

    // 三角形索引表：每 3 个 int 组成一个三角形。
    // 用「拍平的一维数组」而不是 int[][3]：可以直接塞进 std::vector，
    // 也能和 OBJ 那种「先读进来再统计数量」的流程配合。
    std::vector<int> indices;

    // 模型空间的包围盒，加载完立刻算一次 —— 视锥剔除要用（scene/scene.cpp）
    Aabb bounds;

    int vertex_count()   const { return static_cast<int>(positions.size()); }
    int triangle_count() const { return static_cast<int>(indices.size()) / 3; }

    bool empty() const { return positions.empty() || indices.empty(); }

    // ---- 归一化：把模型「居中到原点 + 缩放到统一大小」 ----
    //
    // 为什么需要：不同来源的 .obj 尺度能差好几个数量级 —— 角色模型大概 1.4（米）、
    // 一个零件 0.001、整个场景 10000。相机只有一套参数，不可能为每个模型重调；
    // 所以加载完先归一化，后面所有代码（相机距离、光照、剔除）就都能用同一套数。
    //
    // 做法：先求包围盒中心、把模型平移到原点，再**等比**缩放，让最长的那条边
    // 正好等于 target_max_extent。三个轴用同一个系数，形状才不会变形。
    void normalize(float target_max_extent = 2.0f)
    {
        if (positions.empty()) {
            return;
        }
        if (!bounds.is_valid()) {
            bounds = Aabb::from_positions(positions.data(), vertex_count());
        }

        const Vec3 center = (bounds.min + bounds.max) * 0.5f;
        const Vec3 size   = bounds.max - bounds.min;
        const float longest = std::max(size.x, std::max(size.y, size.z));
        const float s = (longest > kEpsilon) ? (target_max_extent / longest) : 1.0f;

        for (Vec3& p : positions) {
            p = (p - center) * s;
        }
        // 等比缩放下法线方向不变，但长度会跟着变 s 倍 —— 光照要求单位向量，
        // 所以重新归一化一遍（光照不归一化的话，模型会整体偏亮或偏暗）。
        for (Vec3& n : normals) {
            n = n.normalize();
        }

        bounds = Aabb::from_positions(positions.data(), vertex_count());
    }
};

#endif
