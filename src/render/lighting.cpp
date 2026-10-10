#include "lighting.hpp"

#include <algorithm>   // std::max, std::min

Color compute_lambert(const Vec3& normal,
                      const Vec3& light_dir,
                      const Color& base,
                      float ambient)
{
    // diffuse: 0~1，朝向光源=1，背对=0
    float diffuse = std::max(0.0f, normal.dot(light_dir));
    // total: ambient + (1-ambient) * diffuse，范围 [ambient, 1]
    float intensity = ambient + (1.0f - ambient) * diffuse;

    // 钳制到 [0, 255]，防止溢出
    auto clamp255 = [](uint8_t base, float t) -> uint8_t {
        return static_cast<uint8_t>(std::min(255.0f, base * t));
    };
    return Color(
        clamp255(base.r, intensity),
        clamp255(base.g, intensity),
        clamp255(base.b, intensity),
        base.a);
}
