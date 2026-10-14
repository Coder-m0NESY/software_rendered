#include "scene/scene.hpp"

#include "scene/frustum.hpp"

#include <algorithm>   // std::stable_sort

int Scene::add_object(const SceneObject& object)
{
    objects_.push_back(object);
    return static_cast<int>(objects_.size()) - 1;
}

int Scene::cull(const Mat44& view_projection)
{
    // 视锥只跟相机有关，一帧算一次，所有物体共用
    const Frustum frustum = Frustum::from_view_projection(view_projection);

    int visible = 0;
    for (SceneObject& obj : objects_) {
        if (obj.mesh == nullptr) {
            obj.visible = false;
            continue;
        }

        // 包围盒存的是模型空间，先搬到世界空间再比。
        // model 是单位矩阵时 transformed() 基本是白跑，但代码只有一条路径最好维护。
        const Aabb world_bounds = obj.mesh->bounds.transformed(obj.model);

        obj.visible = frustum.intersects(world_bounds);
        if (obj.visible) {
            ++visible;
        }
    }
    return visible;
}

int Scene::visible_count() const
{
    int n = 0;
    for (const SceneObject& obj : objects_) {
        if (obj.visible) {
            ++n;
        }
    }
    return n;
}

std::vector<int> Scene::build_draw_order(const Vec3& eye) const
{
    std::vector<int> opaque;
    std::vector<int> transparent;

    // ① 先按「透不透明」分成两拨。分成两拨而不是边遍历边排序，
    //    是因为最后要保证「不透明整体在前、半透明整体在后」，
    //    这个分组关系比大小关系更硬，排序解决不了。
    for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
        const SceneObject& obj = objects_[i];
        if (!obj.visible) continue;
        (obj.transparent ? transparent : opaque).push_back(i);
    }

    // ② 半透明的那拨按距离从远到近排。
    //
    //    distance 用「世界包围盒中心 → 眼睛」的平方距离。
    //    用中心而不是最近点：反正只是个排序用的近似值，
    //    真实渲染里这一层从来都是近似（三角形级排序才算准，但那太贵）。
    auto distance_sq = [&](int index) -> float {
        const SceneObject& obj = objects_[index];
        const Vec3 center = (obj.mesh->bounds.min + obj.mesh->bounds.max) * 0.5f;
        const Vec3 world_center = (obj.model * center).xyz();
        return (world_center - eye).length_squared();
    };

    //    stable_sort：距离相同时保持原顺序，画面不会无端抖动。
    //    用 > 而不是 <，就是从远到近。
    std::stable_sort(transparent.begin(), transparent.end(),
                     [&](int a, int b) { return distance_sq(a) > distance_sq(b); });

    // ③ 拼起来：不透明 + 半透明（从远到近）
    opaque.insert(opaque.end(), transparent.begin(), transparent.end());
    return opaque;
}
