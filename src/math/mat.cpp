#include "mat.hpp"

#include <cmath>

// Mat44 / Mat33 的实现。声明在 mat.hpp。
//
// 行主序：m[行][列]。这里所有公式都按这个约定写，
// 如果哪天换成列主序，下面每一个 m[i][j] 都得跟着转置。

// ============================================================================
// Mat44 ── 基本变换
// ============================================================================
Mat44 Mat44::identity()
{
    Mat44 r;
    r.m[0][0] = 1.0f;
    r.m[1][1] = 1.0f;
    r.m[2][2] = 1.0f;
    r.m[3][3] = 1.0f;
    return r;
}

Mat44 Mat44::translate(float tx, float ty, float tz)
{
    // 平移写在第四列：只有这一列和点的 w = 1 相乘，才能产生「位移」
    Mat44 r = identity();
    r.m[0][3] = tx;
    r.m[1][3] = ty;
    r.m[2][3] = tz;
    return r;
}

Mat44 Mat44::scale(float sx, float sy, float sz)
{
    Mat44 r;
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    r.m[2][2] = sz;
    r.m[3][3] = 1.0f;
    return r;
}

Mat44 Mat44::rotateX(float angle_rad)
{
    float c = std::cos(angle_rad);
    float s = std::sin(angle_rad);

    // 绕 X 轴转：x 不动，在 yz 平面里转
    Mat44 r = identity();
    r.m[1][1] =  c; r.m[1][2] = -s;
    r.m[2][1] =  s; r.m[2][2] =  c;
    return r;
}

Mat44 Mat44::rotateY(float angle_rad)
{
    float c = std::cos(angle_rad);
    float s = std::sin(angle_rad);

    // 绕 Y 轴转：y 不动，在 xz 平面里转
    Mat44 r = identity();
    r.m[0][0] =  c; r.m[0][2] =  s;
    r.m[2][0] = -s; r.m[2][2] =  c;
    return r;
}

Mat44 Mat44::rotateZ(float angle_rad)
{
    float c = std::cos(angle_rad);
    float s = std::sin(angle_rad);

    // 绕 Z 轴转：z 不动，在 xy 平面里转
    Mat44 r = identity();
    r.m[0][0] =  c; r.m[0][1] = -s;
    r.m[1][0] =  s; r.m[1][1] =  c;
    return r;
}

// ============================================================================
// Mat44 ── 相机相关
// ============================================================================
Mat44 Mat44::perspective(float fov_y_rad, float aspect,
                         float near_z, float far_z)
{
    // f 是焦距：视锥在近平面上的半高。
    // 除以 aspect 是因为 x 方向的张角比 y 方向大，两轴要各自缩放
    float f = 1.0f / std::tan(fov_y_rad * 0.5f);

    Mat44 r;
    r.m[0][0] = f / aspect;
    r.m[1][1] = f;
    r.m[2][2] = (far_z + near_z) / (near_z - far_z);
    r.m[2][3] = (2.0f * near_z * far_z) / (near_z - far_z);
    // 关键的一行：w = -z。透视除法（除以 w）就发生在这之后
    r.m[3][2] = -1.0f;
    // 注意 m[3][3] 故意保持 0：新 w 要完全由 z 决定（w = -z * 1 + 0）。
    // 若在这里补个 1，w 就变成 1 - z，近大远小的效果就没了
    return r;
}

Mat44 Mat44::orthographic(float left, float right,
                          float bottom, float top,
                          float near_z, float far_z)
{
    // 把长方体 [l,r]×[b,t]×[n,f] 线性地拉成 [-1,1]³，没有任何除法
    Mat44 r = identity();
    r.m[0][0] =  2.0f / (right - left);
    r.m[1][1] =  2.0f / (top - bottom);
    r.m[2][2] = -2.0f / (far_z - near_z);

    r.m[0][3] = -(right + left) / (right - left);
    r.m[1][3] = -(top + bottom) / (top - bottom);
    r.m[2][3] = -(far_z + near_z) / (far_z - near_z);
    return r;
}

Mat44 Mat44::lookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
{
    // 相机自己的三个轴，在「世界空间」里表示出来
    Vec3 f = normalize(target - eye);   // 前方
    Vec3 s = normalize(cross(f, up));   // 右方（用叉积把 up 掰正，保证与 f 垂直）
    Vec3 u = cross(s, f);               // 上方：f、s 已是单位且垂直，u 自然也是单位向量

    // 旋转部分：把相机轴填进矩阵的行里，等于让世界「反向旋转」
    // 平移部分：把相机位置搬到原点，所以是 -dot(轴, eye)
    //
    // 为什么填「行」而不是「列」：矩阵乘向量 = 三行分别和向量做点积，
    // 而点积算的是「投影长度」。正交的三个轴互为投影 0，于是
    // s 只在第 0 行留下 1，在另外两行留下 0 —— 正好把 s 映射到 (1,0,0)。
    // 推导过程见 docs/tutorial/lookAt-视图矩阵推导.html
    Mat44 r = identity();
    r.m[0][0] =  s.x; r.m[0][1] =  s.y; r.m[0][2] =  s.z; r.m[0][3] = -dot(s, eye);
    r.m[1][0] =  u.x; r.m[1][1] =  u.y; r.m[1][2] =  u.z; r.m[1][3] = -dot(u, eye);
    // 第三行取负号：相机是朝 -Z 看的，所以世界的 -f 才是视空间的前方
    r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z; r.m[2][3] =  dot(f, eye);
    return r;
}

// ============================================================================
// Mat44 ── 运算
// ============================================================================
Mat44 Mat44::operator*(const Mat44& other) const
{
    // 行主序下的矩阵乘法：结果的 (i,j) = 左矩阵第 i 行 · 右矩阵第 j 列
    Mat44 r;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
                sum += m[i][k] * other.m[k][j];
            r.m[i][j] = sum;
        }
    return r;
}

Vec4 Mat44::operator*(const Vec3& p) const
{
    // 点补成齐次坐标 (x, y, z, 1)，平移才会作用在它身上
    return (*this) * Vec4(p, 1.0f);
}

Vec3 Mat44::transformDirection(const Vec3& d) const
{
    // 只乘左上 3x3，碰都不碰第四列（平移）。
    // 第四列干脆不参与，比「传 w = 0 进去」更直白，也少一次乘加
    return Vec3(m[0][0] * d.x + m[0][1] * d.y + m[0][2] * d.z,
                m[1][0] * d.x + m[1][1] * d.y + m[1][2] * d.z,
                m[2][0] * d.x + m[2][1] * d.y + m[2][2] * d.z);
}

// ---- Mat44 × Vec4 ----
// 声明在 vec.hpp（实现在这里），这样能写 M * v 而不是 v * M
Vec4 operator*(const Mat44& mm, const Vec4& v)
{
    // 结果的第 i 个分量 = 矩阵第 i 行 · v
    return Vec4(
        mm.m[0][0] * v.x + mm.m[0][1] * v.y + mm.m[0][2] * v.z + mm.m[0][3] * v.w,
        mm.m[1][0] * v.x + mm.m[1][1] * v.y + mm.m[1][2] * v.z + mm.m[1][3] * v.w,
        mm.m[2][0] * v.x + mm.m[2][1] * v.y + mm.m[2][2] * v.z + mm.m[2][3] * v.w,
        mm.m[3][0] * v.x + mm.m[3][1] * v.y + mm.m[3][2] * v.z + mm.m[3][3] * v.w);
}

// ============================================================================
// Mat33 ── 2D 变换
// ============================================================================
Mat33 Mat33::identity()
{
    Mat33 r;
    r.m[0][0] = 1.0f;
    r.m[1][1] = 1.0f;
    r.m[2][2] = 1.0f;
    return r;
}

Mat33 Mat33::translate(float tx, float ty)
{
    // 2D 平移写在第三列，和 (x, y, 1) 的 1 相乘
    Mat33 r = identity();
    r.m[0][2] = tx;
    r.m[1][2] = ty;
    return r;
}

Mat33 Mat33::scale(float sx, float sy)
{
    Mat33 r;
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    r.m[2][2] = 1.0f;
    return r;
}

Mat33 Mat33::rotate(float angle_rad)
{
    float c = std::cos(angle_rad);
    float s = std::sin(angle_rad);

    Mat33 r = identity();
    r.m[0][0] =  c; r.m[0][1] = -s;
    r.m[1][0] =  s; r.m[1][1] =  c;
    return r;
}

Mat33 Mat33::operator*(const Mat33& other) const
{
    Mat33 r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
        {
            float sum = 0.0f;
            for (int k = 0; k < 3; ++k)
                sum += m[i][k] * other.m[k][j];
            r.m[i][j] = sum;
        }
    return r;
}

Vec2 Mat33::operator*(const Vec2& p) const
{
    // 把点补成齐次坐标 (x, y, 1) 再相乘
    float x = m[0][0] * p.x + m[0][1] * p.y + m[0][2];
    float y = m[1][0] * p.x + m[1][1] * p.y + m[1][2];
    float w = m[2][0] * p.x + m[2][1] * p.y + m[2][2];

    // 透视除法：仿射矩阵的 w 恒为 1，只有带透视的矩阵才需要除
    if (std::fabs(w) > kEpsilon)
    {
        x /= w;
        y /= w;
    }
    return Vec2(x, y);
}

void Mat33::transformPoint(float& x, float& y) const
{
    Vec2 p = (*this) * Vec2(x, y);
    x = p.x;
    y = p.y;
}

// ---- 两个 3D 投影的转发 ----
// 声明挂在 Mat33 名下是为了跟教程里的调用点对上，但 3×3 矩阵根本表达不了
// 透视投影（它必须靠第 4 个分量才有近大远小），所以实际返回的是 Mat44
Mat44 Mat33::perspective(float fov_y_rad, float aspect,
                         float near_z, float far_z)
{
    return Mat44::perspective(fov_y_rad, aspect, near_z, far_z);
}

Mat44 Mat33::orthographic(float left, float right,
                          float bottom, float top,
                          float near_z, float far_z)
{
    return Mat44::orthographic(left, right, bottom, top, near_z, far_z);
}