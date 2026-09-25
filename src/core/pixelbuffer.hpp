#ifndef PIXELBUFFER_HPP
#define PIXELBUFFER_HPP

#include <SDL.h>
#include <cstdint>
#include <vector>

class Color;

// CPU 端的一块像素画布，底下是一张 32 位的 SDL_Surface。
//
// 像素格式固定成 ARGB8888（也就是 Color::to_pixel() 吐出来的 0xAARRGGBB），
// 跟 window.cpp 里那块纹理的格式一致，所以算完的像素可以直接丢给
// Window::present()，中间不用再转格式。
//
// 所有权：构造函数接管传进来的 SDL_Surface，析构时负责 SDL_FreeSurface。
// 所以别把别处还在引用的 surface 递进来。
//
// 所有写像素的接口都自带越界检查，画到画布外面不会崩，也不会污染别的内存。
//
// 除了颜色，它还带一块**深度缓冲**（每个像素一个 float），也就是管线图里
// 「帧缓冲」那一格的两半。两块缓冲同尺寸、同生命周期，clear() 会把它们一起刷掉。
class PixelBuffer
{
public:
    // 自己建一块 width x height 的画布，建不出来就抛 std::runtime_error
    PixelBuffer(int width, int height);

    // 接管一块已有的 surface（析构时会被 SDL_FreeSurface）。
    // explicit：挡住 SDL_Surface* 被隐式转成 PixelBuffer 这种误用。
    explicit PixelBuffer(SDL_Surface* surface);
    ~PixelBuffer();

    // 手里攥着一块 surface，允许拷贝的话两个对象析构时会重复 free，
    // 所以拷贝构造函数和赋值运算符直接禁掉。
    PixelBuffer(const PixelBuffer&) = delete;
    PixelBuffer& operator=(const PixelBuffer&) = delete;

    // 移动语义：所有权转走，被移动的那个变成空壳（is_valid() 为 false）
    PixelBuffer(PixelBuffer&& other) noexcept;
    PixelBuffer& operator=(PixelBuffer&& other) noexcept;

    // ---- 读写像素：越界一律忽略，不报错也不崩 --------------------------

    // color 是 0xAARRGGBB 的裸像素值
    void put_pixel(int x, int y, uint32_t color);
    void put_pixel(int x, int y, const Color& color);

    // 越界时返回 0（透明黑）
    uint32_t get_pixel(int x, int y) const;
    Color get_pixel_color(int x, int y) const;

    // ---- 深度缓冲 ------------------------------------------------------
    //
    // 约定：深度存的是**裁剪空间 z 除以 w**，也就是 NDC 的 z，范围 [-1, 1]。
    // 投影矩阵把近平面映射成 -1、远平面映射成 +1，所以 **z 越小越近**。
    // 深度测试就一句话：新片元的深度比这里已存的更小，才允许覆盖。
    //
    // 注意别把「视空间的 z」存进来。视空间的 z 在屏幕空间不能线性插值，
    // 插出来的深度是错的；而 z/w 在屏幕空间正好是线性的（它是 1/z 的仿射函数）。

    // 整块刷成最远值（+1），表示「这里还什么都没有」。
    // 于是任何真实片元都比它小，都能通过第一次深度测试。
    void clear_depth();

    // 越界时返回最远值（+1）而不是 0，这样越界像素永远通不过深度测试，
    // 不会因为「看起来最近」而在画布外面画出东西
    float depth_at(int x, int y) const;

    // 写入深度，越界忽略
    void set_depth(int x, int y, float depth);

    // ---- 批量绘图 ------------------------------------------------------

    // 实心矩形，超出画布的部分自动裁掉
    void draw_filled_rect(int x, int y, int w, int h, uint32_t color);
    void draw_filled_rect(int x, int y, int w, int h, const Color& color);

    // 空心矩形（只画四条边），同样自动裁剪
    void draw_rect(int x, int y, int w, int h, uint32_t color);
    void draw_rect(int x, int y, int w, int h, const Color& color);

    // 拿一个颜色铺满整块画布，并把深度缓冲一起刷成最远值。
    // 深度跟着一起清是刻意的——不然上一帧的深度会把这一帧的片元全挡掉。
    void clear(uint32_t color);
    void clear(const Color& color);

    // ---- 查询属性 ------------------------------------------------------
    // 注意：下面这些都假定对象还有效（is_valid() 为 true）。
    // 被 move 走的对象只剩个 nullptr，再去取宽高就是解空指针。

    int width()  const { return surface_->w; }
    int height() const { return surface_->h; }
    int pitch()  const { return surface_->pitch; }   // 单位是字节

    SDL_Surface*       surface()       { return surface_; }
    const SDL_Surface* surface() const { return surface_; }
    SDL_PixelFormat*   format()        { return surface_->format; }
    int  bytes_per_pixel() const { return surface_->format->BytesPerPixel; }

    bool is_valid() const { return surface_ != nullptr; }

private:
    // 越界、或者对象已经被 move 空，都返回 false
    bool in_bounds(int x, int y) const;

    SDL_Surface* surface_ = nullptr;

    // 每个像素一个深度值，紧密排列：下标 = y * 宽 + x。
    // 它的 stride 就是 width()，跟颜色缓冲的 pitch 不是一回事——
    // pitch 是字节数、还可能带行对齐的填充，别混着用。
    std::vector<float> depth_;
};

#endif