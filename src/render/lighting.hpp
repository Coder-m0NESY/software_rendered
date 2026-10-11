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

// 只要亮度（0~1），不要颜色。
//
// 贴图模式下用这个：基色由贴图提供，光照只负责「压暗」，
//   最终颜色 = 贴图像素 × 这个亮度
// 所以不需要先乘一个白色再除回来，直接拿亮度最省事。
float lambert_intensity(const Vec3& normal,
                        const Vec3& light_dir,
                        float ambient = 0.3f);

#endif
