#include "pixelbuffer.hpp"

#include "color.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

// ===========================================================================
//  PixelBuffer 的实现。
//  pixelbuffer.hpp 里只有"这个类长什么样"；跟 SDL_Surface 打交道的活儿全在这里。
// ===========================================================================

namespace {

// 画布的像素格式。挑 ARGB8888 是因为它跟 Color::to_pixel() 的 0xAARRGGBB
// 完全对得上；换成 RGBA8888 的话红蓝会互换，alpha 也会跑到错误的字节上。
constexpr uint32_t kPixelFormat = SDL_PIXELFORMAT_ARGB8888;

// 深度缓冲的清屏值：最远。
// 深度存的是 NDC 的 z（范围 [-1,1]，近平面 -1、远平面 +1），越小越近，
// 所以「什么都没有」必须用一个比任何真实片元都大的值——就是 +1。
constexpr float kDepthFar = 1.0f;

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

    // 颜色和深度两块缓冲同尺寸、同寿命，所以在这里一起建出来。
    // 分两处建的话，迟早有一处会忘了跟着改。
    depth_.assign(static_cast<std::size_t>(width) * height, kDepthFar);
}

PixelBuffer::PixelBuffer(SDL_Surface* surface)
    : surface_(surface)
{
    if (surface_ == nullptr) {
        throw std::runtime_error("传入的 SDL_Surface 不能为 nullptr");
    }

    // 尺寸以传进来的 surface 为准，别去信调用方口头说的宽高
    depth_.assign(static_cast<std::size_t>(surface_->w) * surface_->h,
                  kDepthFar);
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
    : surface_(other.surface_),
      depth_(std::move(other.depth_))
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
        depth_ = std::move(other.depth_);
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

// ============= 深度缓冲 =============

void PixelBuffer::clear_depth()
{
    // 用 fill 而不是 assign：长度没变，省一次可能的内存重分配
    std::fill(depth_.begin(), depth_.end(), kDepthFar);
}

float PixelBuffer::depth_at(int x, int y) const
{
    // 越界返回最远值（原因见头文件）。
    // 注意这里不能返回 0——0 在 NDC 里是「正中间」，比一半的真实片元都近，
    // 那样越界像素就会通过深度测试，在画布外面画出东西。
    if (!in_bounds(x, y)) {
        return kDepthFar;
    }

    // 深度数组是自己分配的，紧密排列，stride 就是 w（不是 color 那边的 pitch）
    return depth_[static_cast<std::size_t>(y) * surface_->w + x];
}

void PixelBuffer::set_depth(int x, int y, float depth)
{
    if (!in_bounds(x, y)) {
        return;  // 越界就直接忽略，跟 put_pixel 一个态度
    }

    depth_[static_cast<std::size_t>(y) * surface_->w + x] = depth;
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

    // 深度跟着一起清。不清的话，上一帧留下的深度会把这一帧的片元全挡掉，
    // 屏幕直接一片空白——而且很难看出是深度没清导致的。
    clear_depth();
}

void PixelBuffer::clear(const Color& color)
{
    clear(color.to_pixel());
}