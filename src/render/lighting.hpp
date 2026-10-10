#ifndef LIGHTING_HPP
#define LIGHTING_HPP

#include "core/color.hpp"
#include "math/vec.hpp"

// Lambert 漫反射 + ambient 环境光。
//
// final = ambient + (1 - ambient) * max(0, dot(normal, light_dir))
//
// ambient 是保底亮度（0~1），防止背光面全黑看不见。
// 即使面背对光源，至少也有 ambient * base 的亮度。
//
// normal 和 light_dir 都必须是**单位向量**，否则亮度算错。
// light_dir 是「从顶点指向光源」的方向。
Color compute_lambert(const Vec3& normal,
                      const Vec3& light_dir,
                      const Color& base,
                      float ambient = 0.3f);

#endif
