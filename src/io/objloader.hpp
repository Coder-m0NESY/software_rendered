#ifndef OBJLOADER_HPP
#define OBJLOADER_HPP

#include <string>
#include <vector>

#include "scene/mesh.hpp"

// ============================================================================
// io/objloader ── 把 .obj 文件读成 Mesh
//
// 为什么第 12 步才做这件事（学习路线原话）：
//   前面一直在手写立方体 —— 顶点少、格式自己定的，出了错能自己排查。
//   等管线完全跑通、画面也对了，再换真模型。这时候如果出问题，
//   **那就一定是 objloader 的锅**，不用怀疑别处。
//
// .obj 是个纯文本格式，只用四种行：
//   v  x y z           顶点位置
//   vt u v             贴图坐标
//   vn x y z           法线
//   f  a/b/c ...       一个面（3 个或更多角）
// 加上 o / g / usemtl / mtllib / # 注释这些，本项目一律忽略。
//
// 分层：io/ 只依赖 scene/ 和 core/，把「文件格式」这件事关在这一层里。
// Mesh 本身不知道自己是 obj 还是 ply 来的 —— 换个格式只换加载器。
// ============================================================================
namespace io {

// 读一个 .obj，成功时写入 out 并返回 true。
// 失败（文件不存在、读不出任何三角形）时返回 false，out 不动，并打一行原因。
//
// 容错规则（真实 .obj 文件经常缺斤少两）：
//   - 缺 vn：用「这个面自己的法线」现算 → 结果是平面着色（每个面一个法线）
//   - 缺 vt：填 (0,0)
//   - 四边形/多边形：以第 0 个角为扇心，扇形三角化
bool load_obj(const std::string& path, Mesh& out);

// 在几个候选目录里找 .obj 文件（不递归），按文件名排序返回**完整路径**。
//
// 为什么给一串目录：exe 在 build/ 下双击跑时模型在 ../models，
// 在项目根目录用终端跑时在 models —— 两种都该能用，所以按顺序试，
// 谁先命中就用谁。全都命中不了就返回空。
std::vector<std::string> find_obj_files(const std::vector<std::string>& dirs);

}   // namespace io

#endif
