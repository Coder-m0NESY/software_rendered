#ifndef VERTEX_HPP
#define VERTEX_HPP

#include "math/vec.hpp"

// 顶点：一个位置 + 一条法线 + 一组 UV。这三个都要跟着一起插值。
//   位置：决定它画在屏幕哪里
//   法线：光照时算这个顶点有多亮
//   UV  ：这个顶点对应贴图上的哪一点（第 10 步加的）
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;        // (0,0) 是纹理左上角，(1,1) 是右下角
};

#endif
