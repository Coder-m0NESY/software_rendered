#include "vec.hpp"

#include <cmath>

// 这里只放「定义」。声明全在 vec.hpp，想知道某个函数是干嘛的去看头文件。
// 数学层的规矩：只吃数字、吐数字，不认识颜色、像素、SDL、窗口里任何一个东西。

// ============================================================================
// Vec2
// ============================================================================
Vec2 Vec2::operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
Vec2 Vec2::operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
Vec2 Vec2::operator-() const { return Vec2(-x, -y); }
Vec2 Vec2::operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
Vec2 Vec2::operator/(float scalar) const { return Vec2(x / scalar, y / scalar); }

Vec2& Vec2::operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
Vec2& Vec2::operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
Vec2& Vec2::operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
Vec2& Vec2::operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

float Vec2::dot(const Vec2& other) const { return x * other.x + y * other.y; }
float Vec2::cross(const Vec2& other) const { return x * other.y - y * other.x; }
float Vec2::length_squared() const { return x * x + y * y; }
float Vec2::length() const { return std::sqrt(length_squared()); }

Vec2 Vec2::normalize() const
{
    float len = length();
    if (len < kEpsilon) return Vec2(0.0f, 0.0f);   // 零向量没有方向可谈
    return *this / len;
}

Vec2 operator*(float s, const Vec2& v) { return Vec2(s * v.x, s * v.y); }
float dot(const Vec2& a, const Vec2& b) { return a.dot(b); }
float cross(const Vec2& a, const Vec2& b) { return a.cross(b); }
float length(const Vec2& v) { return v.length(); }
Vec2 normalize(const Vec2& v) { return v.normalize(); }


// ============================================================================
// Vec3
// ============================================================================
Vec3 Vec3::operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
Vec3 Vec3::operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
Vec3 Vec3::operator-() const { return Vec3(-x, -y, -z); }
Vec3 Vec3::operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
Vec3 Vec3::operator/(float scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }

// 注意每个复合赋值都要把三个分量都算上。以前 z 就是在这漏掉的
Vec3& Vec3::operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
Vec3& Vec3::operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
Vec3& Vec3::operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
Vec3& Vec3::operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }

float Vec3::dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }

Vec3 Vec3::cross(const Vec3& other) const
{
    // 记忆法：每一项都是「另外两个轴」交错相乘再相减。
    // 结果是向量，它同时垂直于 a 和 b
    return Vec3(y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x);
}

// length_squared 千万别漏任何一个分量：漏了 z 的话，
// Vec3(0,0,5).length() 会得到 0，法线就全错了
float Vec3::length_squared() const { return x * x + y * y + z * z; }
float Vec3::length() const { return std::sqrt(length_squared()); }

Vec3 Vec3::normalize() const
{
    float len = length();
    if (len < kEpsilon) return Vec3(0.0f, 0.0f, 0.0f);
    return *this / len;
}

Vec3 operator*(float s, const Vec3& v) { return Vec3(s * v.x, s * v.y, s * v.z); }
float dot(const Vec3& a, const Vec3& b) { return a.dot(b); }
Vec3 cross(const Vec3& a, const Vec3& b) { return a.cross(b); }
float length(const Vec3& v) { return v.length(); }
Vec3 normalize(const Vec3& v) { return v.normalize(); }


// ============================================================================
// Vec4
// ============================================================================
Vec4 Vec4::operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
Vec4 Vec4::operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
Vec4 Vec4::operator-() const { return Vec4(-x, -y, -z, -w); }
Vec4 Vec4::operator*(float scalar) const { return Vec4(x * scalar, y * scalar, z * scalar, w * scalar); }
Vec4 Vec4::operator/(float scalar) const { return Vec4(x / scalar, y / scalar, z / scalar, w / scalar); }

Vec4& Vec4::operator+=(const Vec4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
Vec4& Vec4::operator-=(const Vec4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
Vec4& Vec4::operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
Vec4& Vec4::operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

float Vec4::dot(const Vec4& other) const { return x * other.x + y * other.y + z * other.z + w * other.w; }
float Vec4::length_squared() const { return x * x + y * y + z * z + w * w; }
float Vec4::length() const { return std::sqrt(length_squared()); }

Vec4 Vec4::normalize() const
{
    float len = length();
    if (len < kEpsilon) return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    return *this / len;
}

Vec3 Vec4::xyz() const { return Vec3(x, y, z); }

Vec4 Vec4::perspectiveDivide() const
{
    // 理论上走到这里的点都满足 w > 0，这层保护是给「裁剪没做对」兜底的：
    // 宁可让个别三角形消失，也不要整屏变成 NaN
    if (std::fabs(w) < kEpsilon) return *this;
    return Vec4(x / w, y / w, z / w, 1.0f);
}

Vec4 operator*(float s, const Vec4& v) { return Vec4(s * v.x, s * v.y, s * v.z, s * v.w); }
float dot(const Vec4& a, const Vec4& b) { return a.dot(b); }
float length(const Vec4& v) { return v.length(); }
Vec4 normalize(const Vec4& v) { return v.normalize(); }


// ============================================================================
// lerp
// ============================================================================
Vec2 lerp(const Vec2& a, const Vec2& b, float t) { return a + (b - a) * t; }
Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }
Vec4 lerp(const Vec4& a, const Vec4& b, float t) { return a + (b - a) * t; }