#ifndef LIGHTING_HPP
#define LIGHTING_HPP

#include "core/color.hpp"
#include "math/vec.hpp"

// Lambert 漫反射光照：max(0, dot(normal, light_dir)) * base_color
//
// normal 和 light_dir 都必须是**单位向量**，否则亮度算错。
// light_dir 是「从顶点指向光源」的方向（和光照公式的另一种约定相反，
// 但这里统一用这个，因为算 light_pos - vertex_pos 就得到）。
Color compute_lambert(const Vec3& normal,
                      const Vec3& light_dir,
                      const Color& base);

#endif
