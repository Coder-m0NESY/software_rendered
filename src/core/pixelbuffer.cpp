#include "pixelbuffer.hpp"

#include "color.hpp"

#include <stdexcept>
#include <string>

// ===========================================================================
//  PixelBuffer 的实现。
//  pixelbuffer.hpp 里只有"这个类长什么样"；跟 SDL_Surface 打交道的活儿全在这里。
// ===========================================================================

namespace {

// 画布的像素格式。挑 ARGB8888 是因为它跟 Color::to_pixel() 的 0xAARRGGBB
// 完全对得上；换成 RGBA8888 的话红蓝会互换，alpha 也会跑到错误的字节上。
constexpr uint32_t kPixelFormat = SDL_PIXELFORMAT_ARGB8888;

// surface 一行有多少个 32 位像素。
// pitch 的单位是字节，必须除以 sizeof(uint32_t) 才能当数组下标用。
inline int row_stride(const SDL_Surface* surface)
{
    return surface->pitch / static_cast<int>(sizeof(uint32_t));
}

}  // namespace

// ============= 构造函数 =============

PixelBuffer::PixelBuffer(int width, int height)
    : surface_(SDL_CreateRGBSurfaceWithFormat(0, width, height, 32,
                                              kPixelFormat))
{
    if (surface_ == nullptr) {
        // 直接抛异常，而不是留一个半死不活的对象让后面每处调用都去判空
        throw std::runtime_error(
            std::string("创建 PixelBuffer 失败: ") + SDL_GetError());
    }
}

PixelBuffer::PixelBuffer(SDL_Surface* surface)
    : surface_(surface)
{
    if (surface_ == nullptr) {
        throw std::runtime_error("传入的 SDL_Surface 不能为 nullptr");
    }
}

PixelBuffer::~PixelBuffer()
{
    if (surface_ != nullptr) {
        SDL_FreeSurface(surface_);
        surface_ = nullptr;  // 免得留下野指针
    }
}

// ============= 移动语义 =============

PixelBuffer::PixelBuffer(PixelBuffer&& other) noexcept
    : surface_(other.surface_)
{
    other.surface_ = nullptr;  // 所有权转移，原对象不再 free 这块内存
}

PixelBuffer& PixelBuffer::operator=(PixelBuffer&& other) noexcept
{
    if (this != &other) {  // 防止自我移动
        if (surface_ != nullptr) {
            SDL_FreeSurface(surface_);
        }
        surface_ = other.surface_;
        other.surface_ = nullptr;
    }
    return *this;
}

// ============= 像素读写 =============

bool PixelBuffer::in_bounds(int x, int y) const
{
    // surface_ 为空说明这块画布已经被 move 走了，什么都别碰
    if (surface_ == nullptr) {
        return false;
    }
    return x >= 0 && x < surface_->w && y >= 0 && y < surface_->h;
}

void PixelBuffer::put_pixel(int x, int y, uint32_t color)
{
    if (!in_bounds(x, y)) {
        return;  // 越界就直接忽略，不崩
    }

    // 软件 surface 不需要锁；只有 SDL_MUSTLOCK 为真的表面才要
    const bool must_lock = SDL_MUSTLOCK(surface_) != 0;
    if (must_lock && SDL_LockSurface(surface_) != 0) {
        return;  // 锁不上就别硬写，这时 pixels 指针可能是不可用的
    }

    uint32_t* pixels = static_cast<uint32_t*>(surface_->pixels);
    pixels[y * row_stride(surface_) + x] = color;

    if (must_lock) {
        SDL_UnlockSurface(surface_);
    }
}

uint32_t PixelBuffer::get_pixel(int x, int y) const
{
    if (!in_bounds(x, y)) {
        return 0;  // 越界当透明黑
    }

    const bool must_lock = SDL_MUSTLOCK(surface_) != 0;
    if (must_lock && SDL_LockSurface(surface_) != 0) {
        return 0;
    }

    const uint32_t* pixels = static_cast<const uint32_t*>(surface_->pixels);
    const uint32_t pixel = pixels[y * row_stride(surface_) + x];

    if (must_lock) {
        SDL_UnlockSurface(surface_);
    }
    return pixel;
}

void PixelBuffer::put_pixel(int x, int y, const Color& color)
{
    // 先转成裸像素值，再复用上面那个重载（它负责越界检查）
    put_pixel(x, y, color.to_pixel());
}

Color PixelBuffer::get_pixel_color(int x, int y) const
{
    return Color::from_pixel(get_pixel(x, y));
}

// ============= 批量绘图操作 =============

void PixelBuffer::draw_filled_rect(int x, int y, int w, int h, uint32_t color)
{
    if (surface_ == nullptr) {
        return;  // 已被 move 走
    }

    // 先把矩形裁到画布里面，再进去填，省得在内层循环里反复判断
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > surface_->w) w = surface_->w - x;
    if (y + h > surface_->h) h = surface_->h - y;
    if (w <= 0 || h <= 0) return;   // 整个矩形都在画布外

    const bool must_lock = SDL_MUSTLOCK(surface_) != 0;
    if (must_lock && SDL_LockSurface(surface_) != 0) {
        return;
    }

    uint32_t* pixels = static_cast<uint32_t*>(surface_->pixels);
    const int stride = row_stride(surface_);

    for (int py = y; py < y + h; ++py) {
        // 行首地址只算一次，内层循环就不用再乘 stride 了
        uint32_t* row = pixels + py * stride;
        for (int px = x; px < x + w; ++px) {
            row[px] = color;
        }
    }

    if (must_lock) {
        SDL_UnlockSurface(surface_);
    }
}

void PixelBuffer::draw_filled_rect(int x, int y, int w, int h, const Color& color)
{
    draw_filled_rect(x, y, w, h, color.to_pixel());
}

void PixelBuffer::draw_rect(int x, int y, int w, int h, uint32_t color)
{
    // 四条边逐点画，越界的点由 put_pixel 自己丢掉
    for (int px = x; px < x + w; ++px) {
        put_pixel(px, y, color);            // 上边
        put_pixel(px, y + h - 1, color);    // 下边
    }
    for (int py = y; py < y + h; ++py) {
        put_pixel(x, py, color);            // 左边
        put_pixel(x + w - 1, py, color);    // 右边
    }
}

void PixelBuffer::draw_rect(int x, int y, int w, int h, const Color& color)
{
    draw_rect(x, y, w, h, color.to_pixel());
}

void PixelBuffer::clear(uint32_t color)
{
    if (surface_ == nullptr) {
        return;
    }

    // SDL_FillRect 会按 surface 自己的格式解释这个像素值。
    // 因为画布格式就是 ARGB8888，所以传 to_pixel() 的结果是对的。
    SDL_FillRect(surface_, nullptr, color);
}

void PixelBuffer::clear(const Color& color)
{
    clear(color.to_pixel());
}
