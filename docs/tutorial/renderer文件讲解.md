# renderer 文件讲解：为什么这样写

> 这份教程只讲一件事：`src/render/renderer.cpp`（连同 `renderer.hpp`）**为什么长这样**。
> 不讲 Bresenham（那是 rasterizer 的事），也不讲 MVP 矩阵怎么算（那是第 5 步的事）。
> 读完你应该能回答三个问题：
> 1. 这个文件的**思路**是什么？
> 2. 它的**框架**（谁调用谁、谁认识谁）是什么？
> 3. 每一段的写法背后，**为什么不那么写**？

---

## 一、先建立整体图景：renderer 在管线里的位置

一帧画面从数据到屏幕的完整链条是：

```
顶点数据 (Vec3 数组)
   │
   │  × MVP 矩阵            ← mat.cpp 提供矩阵
   ▼
裁剪坐标 (Vec4, 带 w)
   │
   │  ÷ w（透视除法）        ← vec.cpp 的 perspectiveDivide()
   ▼
NDC（[-1,1] 的标准立方体）
   │
   │  视口变换               ← 【renderer.cpp 干的事之一】
   ▼
屏幕像素坐标 (int x, int y)
   │
   │  按棱表连线             ← 【renderer.cpp 干的事之二】
   │    每条线调 draw_line
   ▼
PixelBuffer 里的像素        ← rasterizer.cpp 的 draw_line 干的
   │
   │  present()
   ▼
屏幕                        ← window.cpp 干的
```

renderer 管的是中间那一段：**「NDC → 屏幕坐标 → 把点连成线」**。
它上游吃 math 层算好的矩阵，下游调用 rasterizer 画线，自己不碰 SDL。

---

## 二、框架：三个文件各认识谁

第 6 步新增了三个文件，分工是刻意设计的：

```
main.cpp ──────────► renderer.cpp ──────► rasterizer.cpp
  │                      │                      │
  │ 认识 Window/SDL      │ 认识 math + core     │ 只认识 core
  ▼                      ▼                      ▼
window.cpp          mat.hpp/vec.hpp        pixelbuffer.hpp
```

| 文件 | 负责 | **不认识**什么（更重要） |
| :--- | :--- | :--- |
| `rasterizer.cpp` | 两个屏幕坐标 → 填满中间像素 | 矩阵、相机、MVP 全都不认识 |
| `renderer.cpp` | NDC→屏幕坐标的换算 + 按棱表把点交给 draw_line | SDL、窗口、事件 |
| `main.cpp` | 组装一切：建窗口、建画布、定顶点棱表、跑主循环 | （它是总装车间，谁都认识） |

**为什么要这样切？** 看依赖方向：

```
main ──► render ──► core ──► math
```

箭头是单向的。`rasterizer` 不认识矩阵，意味着以后改矩阵算法（比如换列主序）**一行 rasterizer 代码都不用动**；`renderer` 不认识 SDL，意味着哪天把 SDL 换成别的窗口库，renderer 原封不动。

这就是 README 模块表里「它不知道什么」那一列存在的意义——**模块的价值不是它会什么，而是它不用会什么**。

---

## 三、renderer.hpp：接口设计里的取舍

```cpp
namespace renderer {
    void viewport(float ndc_x, float ndc_y, int width, int height,
                  int& out_sx, int& out_sy);

    void draw_wireframe(PixelBuffer& fb, const Mat44& mvp,
                        const Vec3* vertices, int vertex_count,
                        const int (*edges)[2], int edge_count,
                        uint32_t color);
}
```

### 为什么用 namespace 而不是 class？

`Window`、`PixelBuffer` 都是 class，因为它们**有状态**（SDL_Window 指针、surface 指针）。renderer 没有任何状态——每次调用都是「给我数据，我还你结果」，纯函数。给纯函数套 class 只会逼你写 `Renderer r; r.draw(...)` 这种没有意义的仪式。

C++ 里「一组相关纯函数」的标准住处就是 namespace。

### 为什么顶点传 `const Vec3* + count` 而不是 `std::vector<Vec3>`？

两个原因：

1. **不挑容器**。数组、vector、将来 .obj 加载器吐出来的任何连续内存都能传。传 vector 就把调用方绑死在 vector 上了。
2. **const 承诺只读**。调用方不用担心自己的顶点被改。

`const int (*edges)[2]` 这个类型看起来吓人，读法是「指向『两个 int 的数组』的指针」——也就是棱表数组 `edges[12][2]` 退化后的类型。

### 为什么输出参数用 `int& out_sx` 而不是返回一个结构体？

这里其实有个更现代的选择：`struct ScreenPoint { int x, y; };` 返回它。用引用输出参数的理由只有一个——**现在还没定义 ScreenPoint 这个类型**，等到第 7 步三角形填充需要携带更多插值数据时，再定义专门的结构体不迟。当前的写法是「够用的最小设计」，不是终点。

---

## 四、viewport：三行代码，两个决定

```cpp
out_sx = (ndc_x + 1.0f) * 0.5f * width;
out_sy = (1.0f - ndc_y) * 0.5f * height;   // ← 为什么是 1-，不是 +1？
```

**决定 1：映射公式。** NDC 的 x 范围是 [-1,1]，屏幕是 [0,width]。

```
先 +1      → [0, 2]
再 ×0.5    → [0, 1]      （归一化到"百分比"）
再 ×width  → [0, width]  （百分比 × 总像素数 = 第几个像素）
```

**决定 2：y 要翻转。** 这是两个坐标系的朝向矛盾：

```
NDC：  y 向上为正（+1 在顶上）
屏幕： y 向下为正（0 在顶上）
```

直接用 x 的公式套 y，+1 会映射到 `height`（底部）——**画面上下颠倒**。
`1 - ndc_y` 先把方向倒过来：NDC 的 +1（顶）变成 0（屏幕顶），-1（底）变成 2 → 再 ×0.5×height 落到底部。

> 第 6 步最常见的 bug 就是忘了翻转：立方体能画出来，但上下是反的，而且**对称的立方体让你很难发现反了**。验证方法：把立方体往上挪一点（M 加个平移 `translate(0, 1, 0)`），它应该往屏幕上方跑。

---

## 五、draw_wireframe：整个文件的核心，三段论

函数体就三段，对应三个决定。

### 决定 1：顶点只变换一次，棱表按下标查

```cpp
int screen_x[kMaxVertices];
int screen_y[kMaxVertices];

for (int i = 0; i < vertex_count; ++i) {
    const Vec4 clip = mvp * vertices[i];
    ...
    viewport(...);
    screen_x[i] = ...;
    screen_y[i] = ...;
}
```

**反面教材**是这样写：

```cpp
// 错：每条棱单独变换它的两个端点
for (int e = 0; e < edge_count; ++e) {
    Vec4 a = mvp * vertices[edges[e][0]];   // ← 同一个顶点被反复变换
    Vec4 b = mvp * vertices[edges[e][1]];
    ...
}
```

立方体 8 个顶点、12 条棱。反面教材做 24 次矩阵乘法，正解只做 8 次——**每个顶点平均被 3 条棱共用，省掉 2/3 的乘法**。

这个「顶点数组 + 索引数组」的结构不是巧合，它是整个图形学的标准数据布局：GPU 的 vertex buffer + index buffer 就是这个。等第 7 步换成三角形索引（`triangles[12][3]`），同样的结构原样复用。

### 决定 2：`clip.w < 1e-6` 的检查——防「鬼畜线」的最小防护

```cpp
if (clip.w < 1e-6f) {
    visible[i] = false;
    continue;
}
```

**为什么需要它？** 回忆第 5 步：投影矩阵把 `w` 塞成了 `-z_view`（顶点到相机的距离）。

- 顶点在相机**前方**：z_view 是负的 → w 是正的 → 一切正常
- 顶点在相机**背后**：z_view 是正的 → w 是**负的**

w 为负时做透视除法（÷w），x、y 的符号会**翻转**：本来在左边的点算完跑到右边。一条棱如果一个端点在前、一个端点在后，画出来的线会横穿整个屏幕——俗称鬼畜线。

**为什么只是跳过而不是正确处理？** 正确的做法叫**裁剪**：把棱在近平面处截断，丢掉背后那段、保留前面那段。那是后面专门的步骤（Sutherland-Hodgman 算法），代码量不小。现在的立方体离相机 3 个单位，永远碰不到近平面，整条跳过是最小成本的保护。

**为什么和 `1e-6` 比而不是和 `0` 比？** 浮点数没有真正的 0。一个「理论上 w=0」的顶点实际算出来可能是 `1e-9`，÷w 直接变成天文数字坐标。用 epsilon 是把它当 0 处理——`perspectiveDivide()` 里也是同一个思路（`|w| < kEpsilon` 就不除了）。

### 决定 3：固定大小的栈数组 + 截断保护

```cpp
constexpr int kMaxVertices = 1024;
if (vertex_count > kMaxVertices) {
    vertex_count = kMaxVertices;
}
int screen_x[kMaxVertices];   // 栈上，1024 × 4 字节 = 4KB
```

**为什么不用 `new` / `std::vector`？** 这个数组是**帧内临时数据**——用完就扔，下一帧重新算。每帧 new/delete 是堆分配，比栈分配贵，还会慢慢碎片化内存。栈数组进入函数就分配好（实际就是挪一下栈指针），离开自动回收。

**为什么截断而不是报错？** 1024 对立方体（8 个顶点）绰绰有余。真传进来更多， silently 截断会丢模型——这是偷懒，诚实说。等第 9 步加载 .obj（动辄上万顶点）时，这里会换成 `std::vector`，现在没到那个时候。

---

## 六、把三段拼起来：一次完整的数据流

拿棱 `{0, 4}`（远面角 0 → 近面角 4）走一遍：

```
顶点 0: (-1,-1,-1) ──mvp──► clip=(-1.30,-1.73,3.81, w=4)   ✓ w>0 可见
                       ──÷w──► ndc=(-0.325,-0.433,0.952)
                       ──viewport(800×600)──► (sx=270, sy=470)
顶点 4: (-1,-1, 1) ──mvp──► clip=(-1.30,-1.73,1.80, w=2)   ✓ w>0 可见
                       ──÷w──► ndc=(-0.650,-0.866,0.902)
                       ──viewport(800×600)──► (sx=140, sy=40)

draw_line(buffer, 270,470, 140,40, 白色)
   → Bresenham 在这条线上逐个 put_pixel
```

循环 12 次，12 条棱全部画完。一帧结束。

---

## 七、几个可能想问的

**Q：为什么 visible 检查放在顶点循环里，而不是画棱的时候？**
画棱时检查也行，但那样每条棱都要重新想一遍「这个顶点可不可见」。顶点循环里算好存起来，画棱时就是纯查表——跟「顶点只变换一次」是同一个思路：**凡是能提前算的，绝不在内层循环里重复算**。

**Q：为什么 renderer 不顺便把 clear() 和 present() 也包了？**
那样 main 就更短了，但 renderer 就得多认识两样东西：「帧的概念」和「SDL 像素指针」。现在 renderer 的合约很干净：你给我一块**已经清好的**画布和数据，我往里画。画之前干什么、画完之后干什么，是调用方的自由。

**Q：这个文件以后会长成什么样？**
每一步都往里加一个阶段：第 7 步加 `fill_triangle` 的调用、第 8 步把深度测试接进来、再往后加裁剪、背面剔除、纹理。现在的 50 行是骨架，骨架的形状（顶点循环 → 图元循环）会一直保留下去。

---

## 八、自测：不看代码能答出来才算懂

1. 为什么顶点变换循环和画棱循环是两个分开的 for，而不是合成一个？
2. `w < 0` 的顶点不画，防的是什么现象？根子上是什么导致的？
3. viewport 里 `1.0f - ndc.y` 的 `1 -` 防的是什么 bug？
4. 如果哪天要把 SDL 换成 GLFW，renderer.cpp 要改几行？（答案：0 行）
5. `screen_x[]` 为什么开在栈上而不是 new 出来？
