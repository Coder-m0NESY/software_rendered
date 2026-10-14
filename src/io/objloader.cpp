#include "io/objloader.hpp"

#include <algorithm>   // std::sort / std::transform
#include <cctype>      // std::tolower
#include <cstdio>
#include <cstdlib>   // std::atoi
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>   // std::move
#include <vector>

namespace io {

namespace {

// 文件里的一行面：取出来的是「原始下标」，可能还是负数（OBJ 允许相对索引）。
struct CornerRef {
    int v  = 0;   // 1 基；负数是「从末尾往前数」
    int vt = 0;   // 0 表示这一角没写 vt
    int vn = 0;   // 0 表示这一角没写 vn
};

// OBJ 的下标是 1 基，而且正数从 1 开始、负数从 -1 开始往回复数。
//   1  → 第 0 个
//   n  → 第 n-1 个
//   -1 → 最后一个
//   -2 → 倒数第二个
int resolve_index(int raw, int count)
{
    if (raw > 0)  return raw - 1;
    if (raw < 0)  return count + raw;
    return -1;    // 0 或没写
}

// 解析一个角 "v/vt/vn"、"v/vt"、"v//vn" 或 "v"。
CornerRef parse_corner(const std::string& token)
{
    CornerRef ref;
    // 按 '/' 切成 1~3 段。注意 "v//vn" 中间是空串，不能当成数字解析。
    const std::size_t first = token.find('/');
    if (first == std::string::npos) {
        ref.v = std::atoi(token.c_str());
        return ref;
    }

    ref.v = std::atoi(token.substr(0, first).c_str());

    const std::size_t second = token.find('/', first + 1);
    if (second == std::string::npos) {
        // "v/vt"
        ref.vt = std::atoi(token.substr(first + 1).c_str());
        return ref;
    }

    // "v/vt/vn"，vt 段可能是空的
    const std::string vt = token.substr(first + 1, second - first - 1);
    if (!vt.empty()) {
        ref.vt = std::atoi(vt.c_str());
    }
    ref.vn = std::atoi(token.substr(second + 1).c_str());
    return ref;
}

}  // namespace

bool load_obj(const std::string& path, Mesh& out)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::printf("[模型] 打不开 %s\n", path.c_str());
        return false;
    }

    // ---- 先把三种「原材料」分开存，等读完再拼成 Mesh ----
    // OBJ 的三个下标流是**各自独立**的：v、vt、vn 都从 1 开始单独编号。
    // 一个三角形可能写 v=5, vt=2, vn=9，三个数之间毫无关系。
    std::vector<Vec3> positions;
    std::vector<Vec2> uvs;
    std::vector<Vec3> normals;

    // 面的原始角（还没解析成下标）
    std::vector<std::vector<CornerRef>> faces;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream in(line);
        std::string tag;
        in >> tag;

        if (tag == "v") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            in >> x >> y >> z;
            positions.emplace_back(x, y, z);
        } else if (tag == "vt") {
            float u = 0.0f, v = 0.0f;
            in >> u >> v;
            // OBJ 的 v 原点在左下角、向上为正；
            // 本项目 Texture 的 v 原点在左上角、向下为正（见 core/texture.hpp）。
            // 两个约定没有对错，但必须换一次，否则贴图上下颠倒。
            uvs.emplace_back(u, 1.0f - v);
        } else if (tag == "vn") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            in >> x >> y >> z;
            normals.emplace_back(x, y, z);
        } else if (tag == "f") {
            std::vector<CornerRef> face;
            std::string token;
            while (in >> token) {
                face.push_back(parse_corner(token));
            }
            if (face.size() >= 3) {
                faces.push_back(std::move(face));
            }
        }
        // o / g / s / usemtl / mtllib / vn 以外的其它行：忽略
    }

    if (positions.empty() || faces.empty()) {
        std::printf("[模型] %s 里没有可用的几何数据\n", path.c_str());
        return false;
    }

    // ---- 去重表 ----
    // OBJ 的顶点是「位置 / UV / 法线」三条独立索引，而 GPU（和我们的 Mesh）
    // 要的是「一个顶点 = 位置+UV+法线捆绑在一起」的单一索引。
    // 所以要把 (v, vt, vn) 这个三元组当 key 去重：
    //   同一个三元组第二次出现 → 复用第一次生成的顶点下标
    // 这个「解绑 → 重新绑定」的过程叫 vertex de-indexing，是所有模型加载器
    // 都要做的一步。
    std::map<std::tuple<int, int, int>, int> vertex_map;

    std::vector<Vec3> out_pos;
    std::vector<Vec3> out_nrm;
    std::vector<Vec2> out_uv;
    out.indices.clear();

    int flat_face_id = 0;   // 给「缺 vn」的面用的，保证每个面拿到独立的 key

    // 绕序投票：只在「三个角都带 vn」的三角形上统计（见函数末尾的说明）
    std::size_t vote_agree = 0;
    std::size_t vote_total = 0;

    for (const std::vector<CornerRef>& face : faces) {
        // 先把这个面的三个角解析成真正下标，并判断它有没有法线
        struct Corner { int v, vt, vn; };
        std::vector<Corner> corners;
        corners.reserve(face.size());

        bool needs_face_normal = false;
        for (const CornerRef& ref : face) {
            const int vi = resolve_index(ref.v,  static_cast<int>(positions.size()));
            const int ti = resolve_index(ref.vt, static_cast<int>(uvs.size()));
            const int ni = resolve_index(ref.vn, static_cast<int>(normals.size()));
            if (vi < 0 || vi >= static_cast<int>(positions.size())) {
                continue;   // 越界，丢掉这一角
            }
            if (ni < 0) needs_face_normal = true;
            corners.push_back({ vi, ti, ni });
        }
        if (corners.size() < 3) continue;

        // 缺法线 → 用前三个角算一次面法线（这叫平面着色）
        Vec3 face_normal;
        if (needs_face_normal) {
            const Vec3& a = positions[corners[0].v];
            const Vec3& b = positions[corners[1].v];
            const Vec3& c = positions[corners[2].v];
            face_normal = cross(b - a, c - a).normalize();
        }

        // 三角化：以第 0 个角为扇心，拆成 (0, i, i+1)
        for (std::size_t i = 1; i + 1 < corners.size(); ++i) {
            const Corner tri[3] = { corners[0], corners[i], corners[i + 1] };

            // 顺便投一票：这个三角形由绕序算出的几何法线，和文件给的 vn 是否同向。
            // 只统计三个角都带 vn 的三角形 —— 缺 vn 的角用的是这里现算的面法线，
            // 那等于拿绕序和自己比，永远「一致」，投票没有意义。
            if (tri[0].vn >= 0 && tri[1].vn >= 0 && tri[2].vn >= 0) {
                const Vec3 geo = cross(positions[tri[1].v] - positions[tri[0].v],
                                       positions[tri[2].v] - positions[tri[0].v]);
                const Vec3 sn  = normals[tri[0].vn] + normals[tri[1].vn] + normals[tri[2].vn];
                ++vote_total;
                if (dot(geo, sn) > 0.0f) {
                    ++vote_agree;
                }
            }

            for (const Corner& c : tri) {
                // 缺法线的角：给它一个「每个面都不同」的合成 key，
                // 这样它不会跟别的面共用顶点，法线才能保持逐面独立
                const int vn_key = (c.vn >= 0) ? c.vn : (-1 - flat_face_id);

                const auto key = std::make_tuple(c.v, c.vt, vn_key);
                auto it = vertex_map.find(key);
                if (it != vertex_map.end()) {
                    out.indices.push_back(it->second);
                    continue;
                }

                const int new_index = static_cast<int>(out_pos.size());
                out_pos.push_back(positions[c.v]);
                out_uv.push_back(c.vt >= 0 ? uvs[c.vt] : Vec2(0.0f, 0.0f));
                out_nrm.push_back(c.vn >= 0 ? normals[c.vn].normalize() : face_normal);
                vertex_map.emplace(key, new_index);
                out.indices.push_back(new_index);
            }
        }

        if (needs_face_normal) ++flat_face_id;
    }

    if (out.indices.empty()) {
        std::printf("[模型] %s 解析后没有任何三角形\n", path.c_str());
        return false;
    }

    // ---- 绕序自动对齐（让「导入自己的模型」直接能用）----
    // OBJ 里的正面三角形按「逆时针朝外」的惯例写（Blender、Maya 等导出器都是）。
    // 本项目的背面剔除用的是屏幕空间 y 向下的判据，正面恰好是**顺时针朝外**
    // （见 render/rasterizer.cpp 的 back_facing）。两边正好相反。
    //
    // 不处理会怎样：导入的模型被整个翻过来 —— 近处的面被当背面丢掉，你看到的
    // 是盒子的内壁，光照也全反。这里不靠猜：拿文件自带的 vn（朝外）和由绕序算出
    // 的几何法线比一眼，多数同向就说明是标准惯例，整体翻一遍。
    // 全无 vn 的模型 vote_total 为 0，不动它。
    if (vote_total > 0 && vote_agree * 2 > vote_total) {
        const std::size_t tris = out.indices.size() / 3;
        for (std::size_t t = 0; t < tris; ++t) {
            std::swap(out.indices[t * 3 + 1], out.indices[t * 3 + 2]);
        }
        std::printf("[模型] 检测到绕序与渲染器相反，已自动翻转（Blender 等标准导出的正常现象）\n");
    }

    out.positions = std::move(out_pos);
    out.normals   = std::move(out_nrm);
    out.uvs       = std::move(out_uv);
    out.bounds    = Aabb::from_positions(out.positions.data(), out.vertex_count());

    std::printf("[模型] 已加载 %s（%d 顶点，%d 三角形）\n",
                path.c_str(), out.vertex_count(), out.triangle_count());
    return true;
}

std::vector<std::string> find_obj_files(const std::vector<std::string>& dirs)
{
    namespace fs = std::filesystem;

    // 把后缀统一成小写再比，这样 .OBJ / .Obj 也能认。
    auto is_obj = [](const fs::path& p) {
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return ext == ".obj";
    };

    for (const std::string& dir : dirs) {
        // 用带 error_code 的重载：目录不存在/没权限是「正常情况」（就是换个目录再找），
        // 不该抛异常把整个程序掀了。
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) {
            continue;
        }

        std::vector<std::string> found;
        for (const fs::directory_entry& e : fs::directory_iterator(dir, ec)) {
            if (!e.is_regular_file(ec)) {
                continue;
            }
            if (is_obj(e.path())) {
                found.push_back(e.path().string());
            }
        }

        if (!found.empty()) {
            // 按文件名排序：这样「按 1/2/3 选」的顺序在每台机器上都一样，
            // 不会因为文件系统的返回顺序不同而变来变去。
            std::sort(found.begin(), found.end());
            std::printf("[模型] 在 %s 里找到 %d 个 .obj\n", dir.c_str(),
                        static_cast<int>(found.size()));
            return found;
        }
    }

    return {};
}

}   // namespace io