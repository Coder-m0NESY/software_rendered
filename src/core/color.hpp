#ifndef COLOR_HPP
#define COLOR_HPP

#include <cstdint>

// 内部像素格式固定成 0xAARRGGBB，跟 SDL 无关。
// 整个项目只有 window.cpp 认识 SDL，其它模块（math / render）可以放心 include 这个头。
class Color {
public:
    uint8_t r,g,b,a;
    Color():r(0),g(0),b(0),a(0){}
    Color(uint8_t r,uint8_t g,uint8_t b,uint8_t a):r(r),g(g),b(b),a(a){}
    Color(uint8_t r,uint8_t g,uint8_t b):r(r),g(g),b(b),a(255){}
    //~Color(){}

    static Color from_pixel(uint32_t pixel);
    uint32_t to_pixel() const;
};

#endif