#include "lighting.hpp"

#include <algorithm>   // std::max

Color compute_lambert(const Vec3& normal,
                      const Vec3& light_dir,
                      const Color& base)
{
    float intensity = std::max(0.0f, normal.dot(light_dir));
    return Color(
        static_cast<uint8_t>(base.r * intensity),
        static_cast<uint8_t>(base.g * intensity),
        static_cast<uint8_t>(base.b * intensity),
        base.a);
}
