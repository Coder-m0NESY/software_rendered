#include "window.hpp"

// ===========================================================================
//  Window 的实现。
//  window.hpp 里只有"这个类长什么样"；跟 SDL 打交道的活儿全在这里。
// ===========================================================================

Window::Window(const std::string& title, int width, int height)
    : width_(width), height_(height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fail("SDL_Init 失败");
        return;
    }
    sdl_ready_ = true;

    window_ = SDL_CreateWindow(title.c_str(),
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width_, height_,
                               SDL_WINDOW_SHOWN);
    if (window_ == nullptr) {
        fail("创建窗口失败");
        return;
    }

    // 先要硬件加速的渲染器，拿不到就退回软件渲染
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (renderer_ == nullptr) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (renderer_ == nullptr) {
        fail("创建渲染器失败");
        return;
    }

    // 一块流式纹理，当作"显存里的一张画布"。
    // 每帧把 CPU 算好的像素怼进去，再让它铺满窗口。
    texture_ = SDL_CreateTexture(renderer_,
                                 SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING,
                                 width_, height_);
    if (texture_ == nullptr) {
        fail("创建纹理失败");
        return;
    }

    closed_ = false;
}

Window::~Window()
{
    // 释放顺序和创建顺序相反：纹理 -> 渲染器 -> 窗口 -> SDL 本身
    if (texture_  != nullptr) { SDL_DestroyTexture(texture_);   }
    if (renderer_ != nullptr) { SDL_DestroyRenderer(renderer_); }
    if (window_   != nullptr) { SDL_DestroyWindow(window_);     }
    if (sdl_ready_)           { SDL_Quit();                     }

    texture_   = nullptr;
    renderer_  = nullptr;
    window_    = nullptr;
    sdl_ready_ = false;
}

void Window::fail(const std::string& what)
{
    const std::string message = what + "\n\nSDL: " + SDL_GetError();
    closed_ = true;

    // 发布版是 -mwindows，没有控制台，光 SDL_Log 你看不见，弹个框最实在
    if (sdl_ready_) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SoftRenderer",
                                 message.c_str(), nullptr);
    } else {
        SDL_Log("%s", message.c_str());
    }
}

void Window::poll_events()
{
    if (closed_) {
        return;
    }

    mouse_dx_ = 0.0f;
    mouse_dy_ = 0.0f;
    wheel_    = 0.0f;
    keys_pressed_.clear();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                closed_ = true;
                break;

            case SDL_KEYDOWN:
                // repeat != 0 是长按的自动重复，不算"刚按下"
                if (event.key.repeat == 0) {
                    keys_pressed_.push_back(event.key.keysym.sym);
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        closed_ = true;
                    }
                }
                break;

            case SDL_MOUSEMOTION:
                // 只有按住左键拖，才算"绕着模型转"
                if ((event.motion.state & SDL_BUTTON_LMASK) != 0) {
                    mouse_dx_ += static_cast<float>(event.motion.xrel);
                    mouse_dy_ += static_cast<float>(event.motion.yrel);
                }
                break;

            case SDL_MOUSEWHEEL:
                wheel_ += static_cast<float>(event.wheel.y);
                break;

            default:
                break;
        }
    }
}

void Window::present(const std::uint32_t* pixels)
{
    if (closed_ || texture_ == nullptr) {
        return;
    }

    // CPU 缓冲区 -> 纹理，然后把纹理铺满整个窗口
    SDL_UpdateTexture(texture_, nullptr, pixels,
                      width_ * static_cast<int>(sizeof(std::uint32_t)));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

bool Window::key_pressed(SDL_Keycode key) const
{
    for (SDL_Keycode pressed : keys_pressed_) {
        if (pressed == key) {
            return true;
        }
    }
    return false;
}
