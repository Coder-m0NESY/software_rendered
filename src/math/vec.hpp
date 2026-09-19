#ifndef VEC_HPP
#define VEC_HPP

// ---------------------------------------------------------------------------
// 浮点数不要直接跟 0 比相等。
// 例：一个几乎为零的向量，归一化时算出来的长度可能是 1e-9 而不是 0.0f，
// 用 == 判不出来，除下去就是 inf / NaN，画面会整块飘掉。
// ---------------------------------------------------------------------------
inline constexpr float kEpsilon = 1e-8f;

// Mat44 定义在 mat.hpp。这里只提前声明一个名字，
// 好让下面那个「矩阵 × 向量」的自由函数有个参数类型可用。
// 注意：整个 math 层谁都不 include 别人的实现，只声明类型，避免循环包含。
class Mat44;

// ============================================================================
// Vec2 ── 二维向量，也当二维点用
//
// 用途：屏幕坐标、UV、Mat33 的输入输出
// ============================================================================
class Vec2
{
public:
    float x = 0.0f;
    float y = 0.0f;

    // 构造函数留在头文件里：它太平凡了，写在头里编译器才能就地内联。
    // 光栅化那种一帧跑几十万次的循环里，函数调用的开销是要计的
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    // ---- 二元运算符 ----
    Vec2 operator+(const Vec2& other) const;
    Vec2 operator-(const Vec2& other) const;
    Vec2 operator-() const;
    Vec2 operator*(float scalar) const;
    Vec2 operator/(float scalar) const;

    // ---- 复合赋值 ----（返回引用，跟内置类型的行为一致）
    Vec2& operator+=(const Vec2& other);
    Vec2& operator-=(const Vec2& other);
    Vec2& operator*=(float scalar);
    Vec2& operator/=(float scalar);

    // ---- 向量运算 ----
    // 查询类方法一律加 const：函数参数几乎都是 const Vec2&，
    // 不加 const 的话 const 对象连 .length() 都调不了
    float dot(const Vec2& other) const;
    // 2D 叉积的结果是一个数：把 b 逆时针转到 a 所扫过的「有向面积」
    float cross(const Vec2& other) const;
    float length_squared() const;
    float length() const;
    // 零向量没有方向可谈，返回 (0,0)，不会算出 NaN
    Vec2 normalize() const;
};

// 自由函数版：让 `2.0f * v` 和 `dot(a, b)` 这两种写法都能用。
// 实现在 vec.cpp，所以这里不用写 inline——整个程序只有一处定义
Vec2 operator*(float s, const Vec2& v);
float dot(const Vec2& a, const Vec2& b);
float cross(const Vec2& a, const Vec2& b);
float length(const Vec2& v);
Vec2 normalize(const Vec2& v);


// ============================================================================
// Vec3 ── 三维向量，也当三维点用
//
// 用途：模型/世界/相机空间里的位置、方向、法线
// ============================================================================
class Vec3
{
public:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // ---- 二元运算符 ----
    Vec3 operator+(const Vec3& other) const;
    Vec3 operator-(const Vec3& other) const;
    Vec3 operator-() const;
    Vec3 operator*(float scalar) const;
    Vec3 operator/(float scalar) const;

    // ---- 复合赋值 ----
    Vec3& operator+=(const Vec3& other);
    Vec3& operator-=(const Vec3& other);
    Vec3& operator*=(float scalar);
    Vec3& operator/=(float scalar);

    // ---- 向量运算 ----
    float dot(const Vec3& other) const;
    // 3D 叉积结果仍是向量：垂直于 a、b 构成的平面，方向由右手定则定。
    // lookAt 里算相机的右方向就是靠它
    Vec3 cross(const Vec3& other) const;
    float length_squared() const;
    float length() const;
    Vec3 normalize() const;
};

Vec3 operator*(float s, const Vec3& v);
float dot(const Vec3& a, const Vec3& b);
Vec3 cross(const Vec3& a, const Vec3& b);
float length(const Vec3& v);
Vec3 normalize(const Vec3& v);


// ============================================================================
// Vec4 ── 齐次坐标 (x, y, z, w)
//
// 为什么需要第 4 个分量：矩阵乘向量时，第 4 列/行是唯一能产生「平移」和
// 「透视除法」的地方。
//   - 点：w = 1，平移会作用在它身上
//   - 方向：w = 0，平移对它无效（方向不该被搬来搬去）
//   - 乘完投影矩阵后：w = -z，除以 w 就得到近大远小
// ============================================================================
class Vec4
{
public:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    Vec4() = default;
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    // explicit：Vec3 到 Vec4 必须显式写出 w，不然「点和方向」会悄悄混用
    explicit Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    // ---- 二元运算符 ----
    Vec4 operator+(const Vec4& other) const;
    Vec4 operator-(const Vec4& other) const;
    Vec4 operator-() const;
    Vec4 operator*(float scalar) const;
    Vec4 operator/(float scalar) const;

    // ---- 复合赋值 ----
    Vec4& operator+=(const Vec4& other);
    Vec4& operator-=(const Vec4& other);
    Vec4& operator*=(float scalar);
    Vec4& operator/=(float scalar);

    // ---- 向量运算 ----
    float dot(const Vec4& other) const;
    float length_squared() const;
    float length() const;
    Vec4 normalize() const;

    // 丢掉 w，取出前三个分量（要的是方向或普通坐标时用）
    Vec3 xyz() const;

    // ---- 透视除法：把 (x, y, z, w) 变成 (x/w, y/w, z/w, 1) ----
    // 乘完投影矩阵必须做这一步，才是真正的 NDC 坐标。
    // 顺序很重要：必须先裁剪、后除法。相机背后的点 w < 0，除完 x/y 会整块翻号，
    // 屏幕上就会出现那种横穿全屏的「鬼畜三角形」。
    Vec4 perspectiveDivide() const;
};

Vec4 operator*(float s, const Vec4& v);
float dot(const Vec4& a, const Vec4& b);
float length(const Vec4& v);
Vec4 normalize(const Vec4& v);

// ---- Mat44 × Vec4 ----
// 声明在这里、实现在 mat.cpp，这样能写 M * v（而不是 v * M）。
// 放在 vec.hpp 而不是 mat.hpp，是为了让「谁 include 了 vec.hpp 谁就能用」，
// 不用关心 mat.hpp 有没有被包含进来。
// （Vec4、Mat44 的成员全是 public，所以不需要 friend，一个前置声明就够了）
Vec4 operator*(const Mat44& m, const Vec4& v);


// ============================================================================
// 线性插值 lerp(a, b, t) = a + (b - a) * t
//
// t = 0 得到 a，t = 1 得到 b，中间就是按比例混出来的值。
// 三处一定会用到：
//   - 光栅化：按重心坐标插值顶点的法线 / UV / 深度
//   - 裁剪：算新顶点落在边上的哪个位置（t 就是交点比例）
//   - 动画：位置和颜色的过渡
//
// 注意：这不是透视校正插值。屏幕空间上等距的 t，在世界空间里并不等距，
// 所以 UV 和颜色还要额外处理（见学习路线第 7 步）。
// ============================================================================
Vec2 lerp(const Vec2& a, const Vec2& b, float t);
Vec3 lerp(const Vec3& a, const Vec3& b, float t);
Vec4 lerp(const Vec4& a, const Vec4& b, float t);

#endif