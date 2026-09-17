#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <SDL.h>

// 窗口 + 事件循环 + 把一块 CPU 像素缓冲区贴到屏幕上。
//
// 它只管"显示"和"输入"，不知道三角形、矩阵、模型是什么东西。
// 这是整个项目里唯一需要认识 SDL 的模块。
//
// 实现都在 window.cpp 里。
class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    // 窗口手里攥着 SDL 的资源（窗口、渲染器、纹理）。
    // 如果允许拷贝，两个 Window 会抢着释放同一块内存，退出时必崩。
    // 所以直接把拷贝禁掉。
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // 每帧开头调一次：把这一帧攒下的事件全部处理掉，
    // 顺便把鼠标/键盘的输入状态清零重记。
    void poll_events();

    // 每帧末尾调一次：把 ARGB8888 的像素缓冲区贴到窗口上。
    // pixels 至少要有 width() * height() 个元素，每个是 0xAARRGGBB。
    void present(const std::uint32_t* pixels);

    // 窗口被关掉、按了 Esc、或者压根没建起来（比如 SDL_Init 失败）时，都是 true。
    bool closed() const { return closed_; }

    int width()  const { return width_; }
    int height() const { return height_; }

    // ---- 本帧的输入：每调一次 poll_events() 就重新统计一遍 ----------------

    // 按住鼠标左键拖动时的位移，向右 / 向下为正
    float mouse_dx() const { return mouse_dx_; }
    float mouse_dy() const { return mouse_dy_; }

    // 滚轮，向上滚为正
    float wheel() const { return wheel_; }

    // 本帧刚按下的键（长按的自动重复不算）
    bool key_pressed(SDL_Keycode key) const;

private:
    // 创建过程失败时统一走这里：报错 + 把自己标记成"已关闭"
    void fail(const std::string& what);

    SDL_Window*   window_    = nullptr;
    SDL_Renderer* renderer_  = nullptr;
    SDL_Texture*  texture_   = nullptr;   // 那块"显存画布"
    bool          sdl_ready_ = false;     // SDL_Init 是否成功过

    int  width_  = 0;
    int  height_ = 0;
    bool closed_ = true;                  // 默认关着，创建成功才打开

    float mouse_dx_ = 0.0f;
    float mouse_dy_ = 0.0f;
    float wheel_    = 0.0f;

    std::vector<SDL_Keycode> keys_pressed_;
};

#endif
