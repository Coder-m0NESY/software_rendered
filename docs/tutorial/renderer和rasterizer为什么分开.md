# renderer 和 rasterizer：为什么要分两个文件

> 你的三个疑问：
> 1. 每步都改这两个文件，为什么不合成一个？
> 2. 文档说 renderer 的职责是「MVP 变换」，那不是 mat 的事吗？
> 3. renderer 里好像没有 MVP 的内容，是我看漏了吗？

---

## 一、先回答第三个问题：MVP 在哪

MVP 矩阵的**创建**在 main.cpp 里：

```cpp
// main.cpp 第 72-78 行
const Mat44 view = Mat44::lookAt(...);           // mat.cpp 造矩阵
const Mat44 projection = Mat44::perspective(...); // mat.cpp 造矩阵
const Mat44 mvp = projection * view * model;      // mat.cpp 的 operator* 合成
```

然后 main 把合成好的 mvp **传给** renderer：

```cpp
// main.cpp 第 93 行
renderer::draw_filled(buffer, mvp, vertices, 8, triangles, 12, color);
```

renderer 收到的是**已经合成好的矩阵**，它不造矩阵，只**用矩阵乘顶点**：

```cpp
// renderer.cpp 第 70 行
const Vec4 clip = mvp * vertices[i];   // 用矩阵乘顶点，这一步叫"应用 MVP"
```

所以 MVP 的流程是：

```
谁造矩阵        谁用矩阵
main.cpp        renderer.cpp
   │               │
   │  lookAt       │  mvp * vertex
   │  perspective  │  perspectiveDivide
   │  operator*    │  viewport
   ▼               ▼
  mat.cpp         rasterizer.cpp
（提供算法）     （提供画线/填三角形）
```

**mat 提供工具（怎么算矩阵），main 决定用什么参数（相机放哪、fov 多少），renderer 拿着工具和参数去变换顶点。**

---

## 二、回答第二个问题：文档说的「MVP 变换」是什么意思

README 里 renderer 那行写的是：

> 串联整条管线：M → V → P → 裁剪 → 视口 → 光栅化

关键词是**串联**，不是计算。意思是 renderer 是**这些步骤串起来的地方**——顶点从 MVP 变换开始，经过裁剪、视口、最后交给光栅化，整条流程在 renderer 里走。

不是 renderer 负责造矩阵（那是 mat 的事），是 renderer 负责**把矩阵作用到顶点上、把结果交给下一步**。

---

## 三、回答第一个问题：为什么不合成一个文件

确实每步都改两个文件。但它们改的**东西完全不同**：

| 文件 | 改的是什么 | 举例 |
| :--- | :--- | :--- |
| `rasterizer.cpp` | **像素级算法**：怎么填像素 | 加 `draw_triangle`（第 7 步）、加深度比较（第 8 步） |
| `renderer.cpp` | **数据流**：顶点怎么变成屏幕坐标、怎么交给光栅化 | 加 `draw_filled`（第 7 步）、改调用方式（第 8 步） |

具体看第 7 步改了什么：

- **rasterizer.cpp** 加了 `edge_cross`、`fill_triangle`——这是**怎么判断一个像素在不在三角形里**的算法，和矩阵、顶点、管线完全无关
- **renderer.cpp** 加了 `draw_filled`——这是**把顶点数组变成三角形数组、循环调用 fill_triangle**，和像素怎么判断无关

合在一起的话你会得到一个文件，里面既有「边函数公式」又有「顶点变换循环」——两件毫不相关的事挤在一起。

---

## 四、一个更直观的比喻

```
mat.cpp     = 螺丝刀厂（造工具）
rasterizer  = 刷子（涂像素的，不知道在涂什么形状）
renderer    = 工人（拿着螺丝刀和刷子，按流程把顶点变成画面）
main.cpp    = 老板（决定画什么、相机放哪、什么时候画）
```

刷子（rasterizer）不需要知道刷的是墙还是桌子，给它坐标它就刷。工人（renderer）不需要造螺丝刀，拿着用就行。

如果把刷子和工人合在一起，换个工人还得把刷子也重新造一遍。

---

## 五、以后会怎么变

| 步骤 | rasterizer 改什么 | renderer 改什么 |
| :---: | :--- | :--- |
| 6（画线） | 加 `draw_line` | 加 `draw_wireframe` |
| 7（填三角形） | 加 `draw_triangle` | 加 `draw_filled` |
| 8（深度测试） | `draw_triangle` 里加几行深度比较 | 不改 |
| 9（光照） | 不改 | `draw_filled` 里加颜色插值 |
| 10（纹理） | 不改 | 加纹理参数、UV 传递 |

注意第 8 步：**只改 rasterizer，不改 renderer**。如果合成一个文件，你改深度比较的代码旁边就是顶点变换的代码，混在一起容易误伤。

第 9、10 步反过来：**只改 renderer，不改 rasterizer**（rasterizer 只管填像素，颜色从哪来它不关心）。

这就是分开的意义：**两件事独立变化时，不用碰另一个**。
