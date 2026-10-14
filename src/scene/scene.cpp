#include "scene/scene.hpp"

#include "scene/frustum.hpp"

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
        // 包围盒存的是模型空间，先搬到世界空间再比。
        // model 是单位矩阵时 transformed() 基本是白跑，但代码只有一条路径最好维护。
        const Aabb world_bounds = obj.bounds.transformed(obj.model);

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
