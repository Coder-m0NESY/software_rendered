#ifndef VEC_HPP
#define VEC_HPP

#include <cstdint>
#include <cmath>

class Vec2 
{
public:
    float x = 0.0f;
    float y = 0.0f;
    
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    //运算符号
    Vec2 operator+(const Vec2& other) const{return Vec2(x + other.x, y + other.y);};
    Vec2 operator-(const Vec2& other) const{return Vec2(x - other.x, y - other.y);};
    Vec2 operator-() const{return Vec2(-x, -y);};
    Vec2 operator*(float scalar) const{return Vec2(x * scalar, y * scalar);};
    Vec2 operator/(float scalar) const{return Vec2(x / scalar, y / scalar);};

    //
    Vec2 operator+=(const Vec2& other){x += other.x; y += other.y; return *this;};
    Vec2 operator-=(const Vec2& other){x -= other.x; y -= other.y; return *this;};
    Vec2 operator*=(float scalar){x *= scalar; y *= scalar; return *this;};
    Vec2 operator/=(float scalar){x /= scalar; y /= scalar; return *this;};

    //向量运算
    //点积
    float dot(const Vec2& other) {return x * other.x + y * other.y;}
    //叉积
    float cross(const Vec2& other) {return x * other.y - y * other.x;}
    //向量长度平方
    float length_squared() {return x * x + y * y;}
    //向量长度
    float length() {return std::sqrt(length_squared());}
    //向量归一化
    Vec2 normalize() {return *this / length();}
};

// 对称写法：让 2.0f * v 也能用（类内只能写 v * 2.0f）
inline Vec2 operator*(float s, const Vec2& v) { return Vec2(s * v.x, s * v.y); }
inline float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline float cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }
inline float length(const Vec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
inline Vec2 normalize(const Vec2& v) {
    float len = length(v);
    // 防止除以零，若向量为零向量则直接返回零向量
    if (len == 0.0f) return Vec2(0.0f, 0.0f);
    return Vec2(v.x / len, v.y / len);
}


class Vec3
{
public:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    //运算符号
    Vec3 operator+(const Vec3& other) const{return Vec3(x + other.x, y + other.y, z + other.z);};
    Vec3 operator-(const Vec3& other) const{return Vec3(x - other.x, y - other.y, z - other.z);};
    Vec3 operator-() const{return Vec3(-x, -y, -z);};
    Vec3 operator*(float scalar) const{return Vec3(x * scalar, y * scalar, z * scalar);};
    Vec3 operator/(float scalar) const{return Vec3(x / scalar, y / scalar, z / scalar);};

    //
    Vec3 operator+=(const Vec3& other){x += other.x; y += other.y; z += other.z; return *this;};
    Vec3 operator-=(const Vec3& other){x -= other.x; y -= other.y; z -= other.z; return *this;};
    Vec3 operator*=(float scalar){x *= scalar; y *= scalar; return *this;};
    Vec3 operator/=(float scalar){x /= scalar; y /= scalar; z /= scalar; return *this;};

    //向量运算
    //点积
    float dot(const Vec3& other) {return x * other.x + y * other.y + z * other.z;}
    //叉积
    float cross(const Vec3& other) {return x * other.y - y * other.x;}
    //向量长度平方
    float length_squared() {return x * x + y * y;}
    //向量长度
    float length() {return std::sqrt(length_squared());}
    //向量归一化
    Vec3 normalize() {return *this / length();}
};

// 对称写法：让 2.0f * v 也能用（类内只能写 v * 2.0f）
inline Vec3 operator*(float s, const Vec3& v) { return Vec3(s * v.x, s * v.y, s * v.z); }
inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float cross(const Vec3& a, const Vec3& b) { return a.x * b.y - a.y * b.x; }  
inline float length(const Vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
inline Vec3 normalize(const Vec3& v) {
    float len = length(v);
    // 防止除以零，若向量为零向量则直接返回零向量
    if (len == 0.0f) return Vec3(0.0f, 0.0f, 0.0f);
    return Vec3(v.x / len, v.y / len, v.z / len);
}


#endif