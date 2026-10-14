#ifndef TEXT_HPP
#define TEXT_HPP

#include <string>

class PixelBuffer;
class Color;

// ============================================================================
//  极简位图文字（5x7 点阵）
//
//  为什么有这个东西：项目里原本没有任何字体，而「左上角列出可选模型」
//  又想直接画在画布上。为此拉一个 SDL_ttf 进来太重，所以自带一张
//  5x7 的点阵表 —— 一共几十个字符，画法就是「按位铺小方块」。
//
//  它只认 ASCII：字母（大小写等价）、数字、以及 . _ - / : ( ) [ ] + = ? !
//  其余字符（比如中文）一律按 '?' 画。
//  ============================================================================
namespace text {

// 一行字的高度（像素）
int line_height(int scale = 1);

// 一串字会占的宽度（像素）。注意末尾那 1 列字距也算进去了。
int width_of(const std::string& s, int scale = 1);

// 在 (x, y) 处画一行字，(x, y) 是左上角。
// 超出去的部分会被底层 put_pixel 的边界检查自然裁掉，不用自己兜。
void draw(PixelBuffer& fb, int x, int y, const std::string& s,
          const Color& color, int scale = 1);

}   // namespace text

#endif
