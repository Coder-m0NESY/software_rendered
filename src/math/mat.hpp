#ifndef MAT_HPP
#define MAT_HPP

#include "vec.hpp"

// ============================================================================
// Mat44 ── 4x4 矩阵，负责 3D 变换（MVP 里的 M / V / P）
//
// 存储：行主序，m[行][列]
// 点用列向量表示，所以「矩阵乘点」写成 M * v，结果的第 i 个分量是：
//     result[i] = m[i][0]*v.x + m[i][1]*v.y + m[i][2]*v.z + m[i][3]*v.w
//
// 多个变换叠加时从左往右乘，作用顺序从右往左（顶点先被 M 作用）
//     clip = P * V * M * v        （先 M 摆到世界，再 V 搬到相机，最后 P 投影）
//
// 坐标系约定：右手系、相机朝 -Z 看、NDC 的 z 范围是 [-1, 1]（OpenGL 风格）
//
// 定义都在 mat.cpp。唯一的例外是 m[4][4] 的默认初始化，那个必须留在头里
// ============================================================================
class Mat44
{
public:
    float m[4][4] = {};

    // ---- 四种基本变换 ----
    static Mat44 identity();
    static Mat44 translate(float tx, float ty, float tz);
    static Mat44 scale(float sx, float sy, float sz);
    static Mat44 rotateX(float angle_rad);   // 注意单位是弧度
    static Mat44 rotateY(float angle_rad);
    static Mat44 rotateZ(float angle_rad);

    // ---- 相机相关 ----
    // 透视投影：视锥 → NDC。把 -z 塞进 w，等后面做透视除法
    static Mat44 perspective(float fov_y_rad, float aspect,
                             float near_z, float far_z);
    // 正交投影：长方体 → NDC（没有近大远小）
    static Mat44 orthographic(float left, float right,
                              float bottom, float top,
                              float near_z, float far_z);
    // 视图矩阵：把世界搬到「相机在原点、朝 -Z 看」的坐标系
    static Mat44 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);

    // ---- 矩阵运算 ----
    Mat44 operator*(const Mat44& other) const;   // 矩阵 × 矩阵（合并变换）
    Vec4  operator*(const Vec3& p) const;        // 矩阵 × 点（隐含 w = 1）

    // 矩阵 × 方向（隐含 w = 0）：只吃旋转和缩放，平移对它无效。
    // 变换法线用它，不用 operator*：法线是方向不是位置，
    // 跟着平移跑掉的话，光照会随着物体移动整体变向
    Vec3 transformDirection(const Vec3& d) const;
};


// ============================================================================
// Mat33 ── 3x3 矩阵，负责 2D 变换（齐次坐标下的平移/旋转/缩放）
//
// 点写成 (x, y, 1)，乘完再除以 w，就能同时表达平移和旋转。
// 3D 部分用不到它，留着给纹理坐标、屏幕空间这类 2D 计算用。
// ============================================================================
class Mat33
{
public:
    float m[3][3] = {};

    // ---- 四种基本变换（静态工厂，直接用 Mat33::xxx() 调用）----
    static Mat33 identity();
    static Mat33 translate(float tx, float ty);
    static Mat33 scale(float sx, float sy);
    static Mat33 rotate(float angle_rad);   // 注意单位是弧度

    // ---- 矩阵运算 ----
    Mat33 operator*(const Mat33& other) const;   // 矩阵 × 矩阵（合并变换）
    Vec2  operator*(const Vec2& p) const;        // 矩阵 × 点（含透视除法）

    // 原地变换一个点（引用参数直接改调用方的变量）
    void transformPoint(float& x, float& y) const;

    // ---- 3D 投影（返回 4x4）----
    // 放在这里是为了和教程的调用点保持一致，实际矩阵是 Mat44 的
    static Mat44 perspective(float fov_y_rad, float aspect,
                             float near_z, float far_z);
    static Mat44 orthographic(float left, float right,
                              float bottom, float top,
                              float near_z, float far_z);
};

#endif