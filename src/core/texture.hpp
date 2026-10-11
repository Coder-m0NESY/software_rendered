#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <cstdint>
#include <vector>

#include "core/color.hpp"

// 纹理：一张放在「UV 空间」里的像素图。
//
// UV 空间约定（本项目自定）：
//   u = 0 在纹理左边，u = 1 在右边；v = 0 在上面，v = 1 在下面。
//   也就是「原点在左上角、v 向下」—— 这样 v 乘高度直接就是行号，不用翻转。
//   （OpenGL 用的是「原点在左下角、v 向上」，要翻转一次。两个约定没有对错，
//     关键是全项目统一。这里把这层约定收在 Texture 内部，外面只管 u、v 落在 [0,1]。）
//
// 采样接口只收 float，不收 Vec2：core 层是底层零件，不许依赖 math 层
// （见学习路线 6.1 / 6.2 的依赖规则），所以这个头文件不 include vec.hpp。
class Texture
{
public:
    Texture(int width, int height, const Color& fill = Color(0, 0, 0));

    int width()  const { return width_; }
    int height() const { return height_; }

    // 取「整数像素」的颜色。x、y 超界会自动绕回另一侧（wrap），
    // 所以调用方不用自己做边界检查。
    Color texel(int x, int y) const;
    void  set_texel(int x, int y, const Color& c);

    // ---- 两种采样方式：u、v 都要求落在 [0,1] ----
    //
    // 最近邻：u、v 落在哪个像素就取哪个。便宜，放大后是马赛克一样的硬边。
    // 双线性：取周围 4 个像素按距离加权平均。贵一点，放大后是平滑过渡。
    Color sample_nearest(float u, float v) const;
    Color sample_bilinear(float u, float v) const;

    // ---- 代码生成一张棋盘格，省得去读图片文件 ----
    // size：纹理边长（像素）；cell：一格的边长（像素）。
    // size / cell 是偶数时，棋盘的四个角颜色一致；奇数则交错，更好看。
    static Texture make_checkerboard(int size, int cell,
                                     const Color& a, const Color& b);

private:
    // 用 ((v % n) + n) % n 而不是 v % n：后者在 v 为负时结果是负的
    static int wrap_index(int v, int n);

    int width_  = 0;
    int height_ = 0;
    std::vector<Color> pixels_;   // 紧密排列，下标 = y * width_ + x
};

#endif