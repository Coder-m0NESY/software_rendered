#include "render/blend.hpp"

#include <algorithm>   // std::clamp

namespace blend {

bool alpha_test(uint8_t alpha, uint8_t threshold)
{
    // 只有一条判据：alpha 达不到阈值就丢掉。
    // 阈值默认 128：0~127 的像素丢、128~255 的留。
    // 阈值可调是有意义的 —— 同一个「树叶」贴图，阈值调低叶子更密、调高叶子更稀疏。
    return alpha >= threshold;
}

Color blend_over(const Color& src, const Color& dst, uint8_t alpha)
{
    const float a  = static_cast<float>(alpha) / 255.0f;
    const float ia = 1.0f - a;

    // 逐通道加权平均。+0.5f 是四舍五入：
    // 直接截断的话，混合会让整幅画面系统性地偏暗一点点，叠多了能看出来。
    auto mix = [a, ia](uint8_t s, uint8_t d) -> uint8_t {
        const float v = static_cast<float>(s) * a + static_cast<float>(d) * ia;
        return static_cast<uint8_t>(std::clamp(v + 0.5f, 0.0f, 255.0f));
    };

    // 结果一律当成不透明（a = 255）：
    // 这一步只负责算出「屏幕上这个像素最终该是什么颜色」。
    // 在本项目这种一帧只画一次、没有多通道合成的方案里，
    // 结果自己带个 alpha 反而会在下一轮混合里被重复计算。
    return Color(mix(src.r, dst.r), mix(src.g, dst.g), mix(src.b, dst.b), 255);
}

}  // namespace blend
