#ifndef VERTEX_HPP
#define VERTEX_HPP

#include "math/vec.hpp"

// 顶点：一个位置 + 一条法线。
// 光照时需要法线来算每个顶点的亮度，然后在三角形内部插值。
struct Vertex {
    Vec3 position;
    Vec3 normal;
};

#endif
