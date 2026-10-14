#include "scene/aabb.hpp"

#include <algorithm>

Aabb::Aabb(const Vec3& min, const Vec3& max) : min(min), max(max) {}

Aabb Aabb::from_positions(const Vec3* positions, int count)
{
    Aabb box;
    if (positions == nullptr || count <= 0) {
        // 空盒：故意让 min > max，is_valid() 就会返回 false
        box.min = Vec3( 1.0f,  1.0f,  1.0f);
        box.max = Vec3(-1.0f, -1.0f, -1.0f);
        return box;
    }

    box.min = positions[0];
    box.max = positions[0];
    for (int i = 1; i < count; ++i) {
        const Vec3& p = positions[i];
        box.min.x = std::min(box.min.x, p.x);
        box.min.y = std::min(box.min.y, p.y);
        box.min.z = std::min(box.min.z, p.z);
        box.max.x = std::max(box.max.x, p.x);
        box.max.y = std::max(box.max.y, p.y);
        box.max.z = std::max(box.max.z, p.z);
    }
    return box;
}

bool Aabb::is_valid() const
{
    return min.x <= max.x && min.y <= max.y && min.z <= max.z;
}

Aabb Aabb::transformed(const Mat44& m) const
{
    if (!is_valid()) {
        return *this;
    }

    // 长方体的 8 个角
    const Vec3 corners[8] = {
        Vec3(min.x, min.y, min.z), Vec3(max.x, min.y, min.z),
        Vec3(min.x, max.y, min.z), Vec3(max.x, max.y, min.z),
        Vec3(min.x, min.y, max.z), Vec3(max.x, min.y, max.z),
        Vec3(min.x, max.y, max.z), Vec3(max.x, max.y, max.z),
    };

    Aabb out;
    for (int i = 0; i < 8; ++i) {
        const Vec3 p = (m * corners[i]).xyz();
        if (i == 0) {
            out.min = p;
            out.max = p;
            continue;
        }
        out.min.x = std::min(out.min.x, p.x);
        out.min.y = std::min(out.min.y, p.y);
        out.min.z = std::min(out.min.z, p.z);
        out.max.x = std::max(out.max.x, p.x);
        out.max.y = std::max(out.max.y, p.y);
        out.max.z = std::max(out.max.z, p.z);
    }
    return out;
}
