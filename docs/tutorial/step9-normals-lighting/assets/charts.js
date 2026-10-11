/* 第 9 步教程 · 插图库
   全部用内联 SVG 手绘，不依赖任何第三方库。
   这些是「几何原理图」，不是数据图表 —— ECharts / Mermaid 都画不了
   需要精确角度和标注的立方体法线图，所以这里直接用 SVG。 */
(function () {
  var NS = 'http://www.w3.org/2000/svg';
  var cs = getComputedStyle(document.documentElement);
  function V(n) { return cs.getPropertyValue(n).trim(); }
  var C = {
    ink: V('--ink'), muted: V('--muted'), rule: V('--rule'),
    accent: V('--accent'), accent2: V('--accent2'),
    bg: V('--bg'), bg2: V('--bg2'),
    far: V('--f-far'), near: V('--f-near'), bottom: V('--f-bottom'),
    top: V('--f-top'), left: V('--f-left'), right: V('--f-right')
  };

  /* ---------- 基础绘图工具 ---------- */
  function el(n, a) {
    var e = document.createElementNS(NS, n);
    for (var k in a) e.setAttribute(k, a[k]);
    return e;
  }
  function root(id, w, h) {
    var host = document.getElementById(id);
    if (!host) return null;
    var s = el('svg', { viewBox: '0 0 ' + w + ' ' + h, class: 'figsvg', xmlns: NS });
    host.appendChild(s);
    return s;
  }
  function txt(g, x, y, s, o) {
    o = o || {};
    var t = el('text', {
      x: x, y: y, fill: o.color || C.ink, 'font-size': o.size || 13,
      'text-anchor': o.anchor || 'middle', 'font-weight': o.weight || 400,
      'font-family': 'inherit'
    });
    t.textContent = s;
    g.appendChild(t);
    return t;
  }
  function line(g, x1, y1, x2, y2, o) {
    o = o || {};
    g.appendChild(el('line', {
      x1: x1, y1: y1, x2: x2, y2: y2, stroke: o.color || C.ink,
      'stroke-width': o.w || 2, 'stroke-dasharray': o.dash || 'none',
      'stroke-linecap': 'round'
    }));
  }
  function arrow(g, x1, y1, x2, y2, o) {
    o = o || {};
    var c = o.color || C.ink, hd = o.head || 8;
    line(g, x1, y1, x2, y2, { color: c, w: o.w || 2, dash: o.dash });
    var a = Math.atan2(y2 - y1, x2 - x1);
    var p1 = [x2 - hd * Math.cos(a - 0.4), y2 - hd * Math.sin(a - 0.4)];
    var p2 = [x2 - hd * Math.cos(a + 0.4), y2 - hd * Math.sin(a + 0.4)];
    g.appendChild(el('polygon', {
      points: x2 + ',' + y2 + ' ' + p1[0] + ',' + p1[1] + ' ' + p2[0] + ',' + p2[1],
      fill: c
    }));
  }
  function poly(g, pts, o) {
    o = o || {};
    g.appendChild(el('polygon', {
      points: pts.map(function (p) { return p[0] + ',' + p[1]; }).join(' '),
      fill: o.fill || 'none', stroke: o.stroke || 'none',
      'stroke-width': o.w == null ? 1.5 : o.w,
      'stroke-dasharray': o.dash || 'none', 'stroke-linejoin': 'round'
    }));
  }
  function circ(g, x, y, r, o) {
    o = o || {};
    g.appendChild(el('circle', {
      cx: x, cy: y, r: r, fill: o.fill || 'none',
      stroke: o.stroke || 'none', 'stroke-width': o.w || 1.5
    }));
  }
  function rbox(g, x, y, w, h, o) {
    o = o || {};
    g.appendChild(el('rect', {
      x: x, y: y, width: w, height: h, rx: o.rx == null ? 10 : o.rx,
      fill: o.fill || 'none', stroke: o.stroke || C.rule, 'stroke-width': o.w || 1
    }));
  }
  function sep(g, w, h) { line(g, 0, h, w, h, { color: C.rule, w: 1 }); }
  function title(g, x, y, s, o) { txt(g, x, y, s, { anchor: 'start', size: 12.5, weight: 600, color: o && o.color || C.accent2 }); }

  /* 灰度：0(黑)~1(白) */
  function gray(t) { var v = Math.round(255 * Math.max(0, Math.min(1, t))); return 'rgb(' + v + ',' + v + ',' + v + ')'; }

  /* ============ 1. 三维坐标系 + 一个方向 ============ */
  (function () {
    var g = root('fig-axes', 340, 300); if (!g) return;
    var ox = 140, oy = 235;
    function P(x, y, z) { return { x: ox + x * 62 + z * 42, y: oy - y * 62 + z * 34 }; }
    var ax = P(1, 0, 0), ay = P(0, 1, 0), az = P(0, 0, 1);
    arrow(g, ox, oy, ax.x, ax.y, { color: C.muted, w: 2 });
    arrow(g, ox, oy, ay.x, ay.y, { color: C.muted, w: 2 });
    arrow(g, ox, oy, az.x, az.y, { color: C.muted, w: 2 });
    txt(g, ax.x + 16, ax.y + 4, 'x (右)', { color: C.muted, size: 12 });
    txt(g, ay.x, ay.y - 10, 'y (上)', { color: C.muted, size: 12 });
    txt(g, az.x + 6, az.y + 16, 'z (前)', { color: C.muted, size: 12 });
    circ(g, ox, oy, 3.5, { fill: C.ink });
    txt(g, ox - 8, oy + 18, '原点', { anchor: 'end', color: C.muted, size: 11 });
    /* 示例方向 (0.6, 0.8, 0)：从原点出发 */
    var d = P(0.6 * 1.35, 0.8 * 1.35, 0);
    arrow(g, ox, oy, d.x, d.y, { color: C.accent, w: 3 });
    txt(g, d.x + 34, d.y - 4, '(0.6, 0.8, 0)', { color: C.accent, size: 12, weight: 600 });
    txt(g, 170, 40, '一组三个数字 = 一个方向', { color: C.ink, size: 13, weight: 600 });
  })();

  /* ============ 2. 方向 vs 位置 ============ */
  (function () {
    var g = root('fig-dir-pos', 520, 210); if (!g) return;
    function up(x, base, label) {
      arrow(g, x, base, x, base - 110, { color: C.accent, w: 3, head: 10 });
      circ(g, x, base, 4, { fill: C.ink });
      txt(g, x, base + 20, label, { color: C.muted, size: 12 });
    }
    up(130, 165, '起点 A');
    up(390, 165, '起点 B');
    rbox(g, 200, 55, 120, 34, { fill: C.bg2 });
    txt(g, 260, 77, '方向完全相同', { size: 12, weight: 600, color: C.ink });
    txt(g, 260, 175, '两处起点不同，但箭头都朝上 —— 方向与画在哪里无关', { size: 12.5, color: C.muted });
  })();

  /* ============ 3. 平面法线（侧视图 + 90°） ============ */
  (function () {
    var g = root('fig-plane-normal', 380, 270); if (!g) return;
    var y = 215, x0 = 45, x1 = 335;
    line(g, x0, y, x1, y, { color: C.ink, w: 4 });
    txt(g, 350, y + 4, '面', { color: C.muted, size: 12, anchor: 'end' });
    var px = 190;
    arrow(g, px, y, px, y - 130, { color: C.accent, w: 3.5, head: 11 });
    txt(g, px + 12, y - 118, '法线', { color: C.accent, size: 13, weight: 700, anchor: 'start' });
    /* 直角标记 */
    line(g, px, y - 22, px + 22, y - 22, { color: C.muted, w: 1.2 });
    line(g, px + 22, y, px + 22, y - 22, { color: C.muted, w: 1.2 });
    txt(g, px + 34, y - 6, '90°', { color: C.muted, size: 12, anchor: 'start' });
    circ(g, px, y, 4, { fill: C.ink });
    txt(g, 190, 40, '面朝上 → 法线就朝上（垂直 + 指向外侧）', { size: 13, weight: 600 });
    /* 不取的方向 */
    arrow(g, px + 90, y, px + 90, y + 90, { color: C.muted, w: 2, dash: '5 5', head: 8 });
    txt(g, px + 96, y + 78, '朝里：不取', { color: C.muted, size: 11.5, anchor: 'start' });
  })();

  /* ============ 4. 平面上多点法线方向相同 ============ */
  (function () {
    var g = root('fig-plane-many', 660, 260); if (!g) return;
    /* 左：正视图（面朝向读者） */
    title(g, 30, 30, '正视图：面朝你');
    rbox(g, 40, 55, 270, 160, { fill: C.bg2, stroke: C.rule, rx: 4 });
    [90, 175, 260].forEach(function (x, i) {
      var cx = x, cy = 135;
      circ(g, cx, cy, 13, { fill: 'none', stroke: C.accent, w: 2 });
      circ(g, cx, cy, 3.5, { fill: C.accent });
      txt(g, cx, cy + 34, '点 ' + 'ABC'[i], { color: C.muted, size: 12 });
    });
    txt(g, 175, 238, '⊙ = 箭头垂直纸面、朝你射出', { color: C.accent, size: 12, weight: 600 });
    sep(g, 330, 240);
    /* 右：侧视图 */
    title(g, 370, 30, '侧视图：面变成一条线');
    var y = 195;
    line(g, 370, y, 640, y, { color: C.ink, w: 4 });
    [405, 500, 595].forEach(function (x, i) {
      arrow(g, x, y, x, y - 92, { color: C.accent, w: 2.5, head: 8 });
      txt(g, x, y + 20, '点 ' + 'ABC'[i], { color: C.muted, size: 12 });
    });
    txt(g, 505, 238, '三个点的法线，方向完全一样', { color: C.ink, size: 12.5, weight: 600 });
  })();

  /* ============ 5. 球面：每点法线都不同 ============ */
  (function () {
    var g = root('fig-sphere', 360, 330); if (!g) return;
    var cx = 180, cy = 160, R = 88;
    circ(g, cx, cy, R, { fill: C.bg2, stroke: C.ink, w: 2 });
    var pts = [
      { a: -90, t: '球顶：朝上' }, { a: 0, t: '球右：朝右' },
      { a: 90, t: '球底：朝下' }, { a: 180, t: '球左：朝左' }
    ];
    pts.forEach(function (p) {
      var rad = p.a * Math.PI / 180;
      var ux = Math.cos(rad), uy = Math.sin(rad);
      var sx = cx + ux * R, sy = cy + uy * R;
      arrow(g, sx, sy, cx + ux * (R + 52), cy + uy * (R + 52), { color: C.accent, w: 3, head: 9 });
      var lx = cx + ux * (R + 70), ly = cy + uy * (R + 70);
      txt(g, lx + (ux > 0.3 ? 6 : ux < -0.3 ? -6 : 0), ly + (uy > 0.5 ? 14 : uy < -0.5 ? -6 : 4), p.t,
        { color: C.accent, size: 11.5, weight: 600, anchor: ux > 0.3 ? 'start' : ux < -0.3 ? 'end' : 'middle' });
    });
    txt(g, 180, 305, '曲面：每个点都有自己的法线方向', { size: 13, weight: 600, color: C.ink });
  })();

  /* ============ 6. 画法线三步 ============ */
  (function () {
    var g = root('fig-draw-steps', 700, 250); if (!g) return;
    var steps = [
      { t: '① 面上任选一个点', sub: '正视图' },
      { t: '② 从该点画垂直箭头', sub: '侧视图' },
      { t: '③ 箭头指向面外侧', sub: '正视图' }
    ];
    steps.forEach(function (s, i) {
      var x0 = 30 + i * 228, y0 = 55, w = 200, h = 130;
      title(g, x0, 35, s.t);
      txt(g, x0 + w, 35, s.sub, { anchor: 'end', size: 11.5, color: C.muted });
      if (i === 1) {
        var y = y0 + 95;
        line(g, x0, y, x0 + w, y, { color: C.ink, w: 4 });
        var px = x0 + w / 2;
        arrow(g, px, y, px, y - 78, { color: C.accent, w: 3, head: 9 });
        line(g, px, y - 18, px + 18, y - 18, { color: C.muted, w: 1.2 });
        line(g, px + 18, y, px + 18, y - 18, { color: C.muted, w: 1.2 });
        circ(g, px, y, 4, { fill: C.ink });
      } else {
        rbox(g, x0, y0, w, h, { fill: C.bg2, stroke: C.rule, rx: 4 });
        var cx = x0 + w / 2, cy = y0 + h / 2;
        if (i === 0) { circ(g, cx, cy, 5, { fill: C.ink }); }
        else {
          circ(g, cx, cy, 15, { fill: 'none', stroke: C.accent, w: 2.5 });
          circ(g, cx, cy, 4.5, { fill: C.accent });
        }
      }
    });
    txt(g, 350, 220, '⊙ = 垂直于纸面、朝你射出的箭头（这就是正视图里的法线画法）', { size: 12, color: C.muted });
    txt(g, 350, 240, '判断标准：箭头和面永远成 90°，且指向外面', { size: 12, color: C.ink, weight: 600 });
  })();

  /* ---------- 等轴测立方体工具 ---------- */
  function cubeProject(cx, cy, sc) {
    return function (x, y, z) {
      return { x: cx + (x - z) * 0.8660 * sc, y: cy + ((x + z) * 0.5 - y) * sc };
    };
  }
  var HIDDEN_FROM = [-1, -1, -1];
  function drawCube(g, P, o) {
    o = o || {};
    var V = [[-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
             [-1, -1, 1], [1, -1, 1], [1, 1, 1], [-1, 1, 1]];
    var hidden = [[0, 1], [0, 4], [0, 3]];
    var isHidden = function (a, b) {
      return hidden.some(function (h) {
        return (h[0] === a && h[1] === b) || (h[0] === b && h[1] === a);
      });
    };
    for (var a = 0; a < 8; a++) {
      for (var b = a + 1; b < 8; b++) {
        var d = 0, same = 0;
        for (var k = 0; k < 3; k++) { if (V[a][k] === V[b][k]) same++; else d += Math.abs(V[a][k] - V[b][k]); }
        if (same !== 2) continue;
        var p1 = P(V[a][0], V[a][1], V[a][2]), p2 = P(V[b][0], V[b][1], V[b][2]);
        if (isHidden(a, b)) line(g, p1.x, p1.y, p2.x, p2.y, { color: C.rule, w: 1.6, dash: '5 5' });
        else line(g, p1.x, p1.y, p2.x, p2.y, { color: C.ink, w: 2.4 });
      }
    }
    /* 三个可见面填色 */
    if (o.fill) {
      var faces = [
        { c: [[-1, 1, -1], [1, 1, -1], [1, 1, 1], [-1, 1, 1]], col: C.top },
        { c: [[-1, 1, 1], [1, 1, 1], [1, -1, 1], [-1, -1, 1]], col: C.near },
        { c: [[1, 1, 1], [1, 1, -1], [1, -1, -1], [1, -1, 1]], col: C.right }
      ];
      faces.forEach(function (f) {
        poly(g, f.c.map(function (v) { return P(v[0], v[1], v[2]); }), { fill: f.col, stroke: f.col, w: 2 });
        g.lastChild.setAttribute('fill-opacity', '0.22');
      });
    }
  }

  /* ============ 7. 立方体 6 个面 = 6 个法线方向 ============ */
  (function () {
    var g = root('fig-cube-faces', 660, 470); if (!g) return;
    var P = cubeProject(300, 230, 74);
    drawCube(g, P, { fill: true });
    var dirs = [
      { n: [0, 0, -1], col: C.far, t: '远面 z=-1 · 红' },
      { n: [0, 0, 1], col: C.near, t: '近面 z=+1 · 绿' },
      { n: [0, -1, 0], col: C.bottom, t: '底面 y=-1 · 蓝' },
      { n: [0, 1, 0], col: C.top, t: '顶面 y=+1 · 黄' },
      { n: [-1, 0, 0], col: C.left, t: '左面 x=-1 · 紫' },
      { n: [1, 0, 0], col: C.right, t: '右面 x=+1 · 青' }
    ];
    var L = 1.15;
    dirs.forEach(function (d) {
      /* 从该面中心出发，沿法线方向 */
      var base = [d.n[0] * 0.9, d.n[1] * 0.9, d.n[2] * 0.9];
      var tip = [base[0] + d.n[0] * L, base[1] + d.n[1] * L, base[2] + d.n[2] * L];
      var p0 = P(base[0], base[1], base[2]), p1 = P(tip[0], tip[1], tip[2]);
      var hiddenFace = d.n[0] === -1 || d.n[1] === -1 || d.n[2] === -1;
      arrow(g, p0.x, p0.y, p1.x, p1.y, { color: d.col, w: 3, head: 10, dash: hiddenFace ? '6 4' : 'none' });
      var ex = p1.x + (p1.x - p0.x) * 0.28, ey = p1.y + (p1.y - p0.y) * 0.28;
      txt(g, ex, ey, d.t, { color: d.col, size: 12, weight: 600 });
    });
    txt(g, 330, 30, '立方体 6 个面 → 6 个不同的法线方向', { size: 14, weight: 700, color: C.ink });
    txt(g, 330, 452, '虚线箭头 = 背面（被挡住）的法线，同样存在', { size: 12, color: C.muted });
  })();

  /* ============ 8. ★ 36 根法线：全部画在同一颗立方体上 ============
     立方体摆「正」（轴对齐等轴测）时，三个可见面的法线在屏幕上只差 60°，
     而且被 3 个面共用的那个角会一次射出 6 根，全部挤在同一点上。
     所以这里做两件事：
       ① 把立方体转歪（绕 Y 28°、绕 X 16°），6 个方向的法线在屏幕上散开；
       ② 每个角上的箭头起点做系统排布：同方向的 2 根沿垂直方向左右错开，
          起点太近的互相推开一点，保证 36 根每一根都单独看得见。
     顶点顺序与 src/main.cpp 的 vertices[36] 完全一致。 */
  (function () {
    var W = 900, H = 620;
    var g = root('fig-cube-36', W, H); if (!g) return;

    var CX = 370, CY = 345, SC = 98, LEN = 64;

    /* 立方体的倾斜：绕 Y 转 YAW，再绕 X 转 PIT（纯旋转，法线长度不变） */
    var YAW = 28 * Math.PI / 180, PIT = 16 * Math.PI / 180;
    var cY = Math.cos(YAW), sY = Math.sin(YAW);
    var cP = Math.cos(PIT), sP = Math.sin(PIT);
    function rot(p) {
      var X = p[0] * cY + p[2] * sY;
      var Z = -p[0] * sY + p[2] * cY;
      var Y = p[1];
      return [X, Y * cP - Z * sP, Y * sP + Z * cP];
    }
    var P = cubeProject(CX, CY, SC);
    function PR(x, y, z) { var r = rot([x, y, z]); return P(r[0], r[1], r[2]); }
    /* 3D 方向 → 屏幕上的单位方向（投影是线性的，直接代进去就行） */
    function dir2(d) {
      var r = rot(d);
      var dx = (r[0] - r[2]) * 0.8660, dy = (r[0] + r[2]) * 0.5 - r[1];
      var L = Math.sqrt(dx * dx + dy * dy);
      return [dx / L, dy / L];
    }

    /* 带白底的文字：压在箭头上也能看清 */
    function tag(x, y, s, o) {
      o = o || {};
      var fs = o.size || 11;
      rbox(g, x - (s.length * fs * 0.62 + 9) / 2, y - fs * 0.85 - 2,
        s.length * fs * 0.62 + 9, fs * 1.15 + 4, { fill: C.bg, stroke: 'none', rx: 3 });
      txt(g, x, y, s, o);
    }

    /* 立方体本体（先画，箭头压在它上面） */
    drawCube(g, PR, { fill: true });

    /* ---- 6 个面：颜色 / 法线方向 / 下标起点 / 是否可见 ---- */
    var FACES = [
      { name: '远面', n: [0, 0, -1], nd: '(0, 0, -1)', col: C.far,    i0: 0,  vis: false },
      { name: '近面', n: [0, 0, 1],  nd: '(0, 0, +1)', col: C.near,   i0: 6,  vis: true },
      { name: '底面', n: [0, -1, 0], nd: '(0, -1, 0)', col: C.bottom, i0: 12, vis: false },
      { name: '顶面', n: [0, 1, 0],  nd: '(0, +1, 0)', col: C.top,    i0: 18, vis: true },
      { name: '左面', n: [-1, 0, 0], nd: '(-1, 0, 0)', col: C.left,   i0: 24, vis: false },
      { name: '右面', n: [1, 0, 0],  nd: '(+1, 0, 0)', col: C.right,  i0: 30, vis: true }
    ];
    /* 每个面的 6 个渲染顶点，顺序与 main.cpp 里 vertices[i0 .. i0+5] 一一对应 */
    var VPTS = [
      [[-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, -1, -1], [1, 1, -1], [-1, 1, -1]],
      [[-1, -1, 1], [-1, 1, 1], [1, 1, 1], [-1, -1, 1], [1, 1, 1], [1, -1, 1]],
      [[-1, -1, -1], [-1, -1, 1], [1, -1, 1], [-1, -1, -1], [1, -1, 1], [1, -1, -1]],
      [[-1, 1, -1], [1, 1, 1], [-1, 1, 1], [-1, 1, -1], [1, 1, -1], [1, 1, 1]],
      [[-1, -1, -1], [-1, -1, 1], [-1, 1, 1], [-1, -1, -1], [-1, 1, 1], [-1, 1, -1]],
      [[1, -1, -1], [1, 1, 1], [1, -1, 1], [1, -1, -1], [1, 1, -1], [1, 1, 1]]
    ];

    /* ---- 先把 36 根法线都做出来 ---- */
    var arrows = [];
    FACES.forEach(function (f, fi) {
      var d = dir2(f.n);
      VPTS[fi].forEach(function (p, k) {
        arrows.push({ b: PR(p[0], p[1], p[2]), d: d, col: f.col,
                      vis: f.vis, face: fi, idx: f.i0 + k, o: [0, 0] });
      });
    });

    /* ---- 同一个角上的箭头排布：不让任何两根完全重合 ---- */
    var corners = {};
    arrows.forEach(function (a) {
      var key = Math.round(a.b.x) + '|' + Math.round(a.b.y);
      (corners[key] = corners[key] || []).push(a);
    });
    Object.keys(corners).forEach(function (key) {
      var list = corners[key];

      /* (1) 同一方向的 2 根（共用角）：沿该方向的垂直方向各挪 R → 两条平行箭头 */
      var byDir = {}, R = 15;
      list.forEach(function (a) {
        var dk = a.d[0].toFixed(3) + '|' + a.d[1].toFixed(3);
        (byDir[dk] = byDir[dk] || []).push(a);
      });
      Object.keys(byDir).forEach(function (dk) {
        var grp = byDir[dk];
        if (grp.length < 2) return;
        var per = [-grp[0].d[1], grp[0].d[0]];
        grp.forEach(function (a, i) {
          var s = i === 0 ? 1 : -1;
          a.o = [per[0] * R * s, per[1] * R * s];
        });
      });

      /* (2) 起点太近的互相推开；起点离角最远不超过 MAXR，仍然贴着这个角 */
      var MIN = 18, MAXR = 26;
      for (var it = 0; it < 40; it++) {
        for (var i = 0; i < list.length; i++) {
          for (var j = i + 1; j < list.length; j++) {
            var A = list[i], B = list[j];
            var dx = A.o[0] - B.o[0], dy = A.o[1] - B.o[1];
            var dd = Math.sqrt(dx * dx + dy * dy);
            if (dd >= MIN) continue;
            if (dd < 0.01) { dx = 1; dy = 0; dd = 1; }
            var push = (MIN - dd) / 2, ux = dx / dd, uy = dy / dd;
            A.o[0] += ux * push; A.o[1] += uy * push;
            B.o[0] -= ux * push; B.o[1] -= uy * push;
          }
        }
        list.forEach(function (a) {
          var L = Math.sqrt(a.o[0] * a.o[0] + a.o[1] * a.o[1]);
          if (L > MAXR) { a.o[0] *= MAXR / L; a.o[1] *= MAXR / L; }
        });
      }
    });

    /* ---- 画箭头：先背面（虚线），再可见面（实线） ---- */
    var labels = [];
    arrows.sort(function (a, b) { return (a.vis ? 1 : 0) - (b.vis ? 1 : 0); })
      .forEach(function (a) {
        var sx = a.b.x + a.o[0], sy = a.b.y + a.o[1];
        var ex = sx + a.d[0] * LEN, ey = sy + a.d[1] * LEN;
        arrow(g, sx, sy, ex, ey, {
          color: a.col, w: a.vis ? 2.6 : 1.8, head: a.vis ? 8 : 6,
          dash: a.vis ? 'none' : '5 4'
        });
        circ(g, sx, sy, a.vis ? 3.2 : 2.2, { fill: a.col });
        if (a.face === 3) {          /* 顶面：把它的 6 个下标标出来当例子 */
          labels.push({ x: ex + a.d[0] * 16, y: ey + a.d[1] * 16 + 4, s: String(a.idx) });
        }
      });
    labels.forEach(function (t) { tag(t.x, t.y, t.s, { color: C.top, size: 11, weight: 700 }); });

    /* ---- 标题 ---- */
    txt(g, CX, 30, '36 根法线，全部画在这一颗立方体上', { size: 16, weight: 700 });
    txt(g, CX, 52, '把立方体转歪一点（绕 Y 28°、绕 X 16°），6 个方向的法线就散开了，不再互相压住。',
      { size: 12, color: C.muted });

    /* ---- 右侧图例 ---- */
    line(g, 636, 92, 636, 578, { color: C.rule, w: 1 });
    rbox(g, 650, 100, 240, 478, { fill: C.bg2, stroke: C.rule });

    txt(g, 662, 128, '6 个面 → 6 个方向 → 36 根', { anchor: 'start', size: 13.5, weight: 700 });
    txt(g, 662, 150, '实线 = 3 个可见面，共 18 根', { anchor: 'start', size: 11.5, color: C.ink });
    txt(g, 662, 170, '虚线 = 3 个背面，同样 18 根', { anchor: 'start', size: 11.5, color: C.muted });

    FACES.forEach(function (f, i) {
      var y = 200 + i * 27;
      if (f.vis) arrow(g, 664, y - 4, 694, y - 4, { color: f.col, w: 2.4, head: 7 });
      else arrow(g, 664, y - 4, 694, y - 4, { color: f.col, w: 1.8, head: 6, dash: '4 3' });
      txt(g, 702, y, f.name + ' ' + f.nd + '　i=' + f.i0 + '~' + (f.i0 + 5),
        { anchor: 'start', size: 11.5, color: f.vis ? C.ink : C.muted });
    });

    line(g, 662, 372, 878, 372, { color: C.rule, w: 1 });
    txt(g, 662, 398, '为什么一个面是 6 根？', { anchor: 'start', size: 12, weight: 700, color: C.ink });
    txt(g, 662, 419, '面 = 2 个三角形 × 3 顶点 = 6 个渲染顶点', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 662, 446, '4 个角为什么变 6 根？', { anchor: 'start', size: 12, weight: 700, color: C.ink });
    txt(g, 662, 467, '共用边两端的 2 个角各被两个三角形用到', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 662, 488, '→ 2 + 1 + 2 + 1 = 6', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 662, 515, '同一个角的 2 根并排画出（错开约 30px），', { anchor: 'start', size: 11.5, color: C.ink });
    txt(g, 662, 536, '否则同方向的箭头会完全重合。', { anchor: 'start', size: 11.5, color: C.ink });
    txt(g, 662, 563, '小数字 = 顶面 6 根的下标 i（18~23）',
      { anchor: 'start', size: 11.5, color: C.top, weight: 600 });
  })();

  /* ============ 9. 8 个几何角 vs 36 个渲染顶点 ============ */
  (function () {
    var g = root('fig-8v36', 700, 330); if (!g) return;
    /* 左：8 个角 */
    var P = cubeProject(175, 160, 62);
    drawCube(g, P, { fill: true });
    var corners = [[-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
                   [-1, -1, 1], [1, -1, 1], [1, 1, 1], [-1, 1, 1]];
    corners.forEach(function (c) {
      var p = P(c[0], c[1], c[2]);
      circ(g, p.x, p.y, 5, { fill: C.accent2, stroke: C.bg, w: 2 });
    });
    title(g, 30, 32, '几何角：8 个');
    txt(g, 175, 308, '立方体确实只有 8 个角', { size: 12, color: C.muted });
    sep(g, 370, 300);
    /* 右：一个面拆成 2 个三角形，6 个渲染顶点 */
    title(g, 410, 32, '一个面 = 2 三角形 × 3 顶点 = 6');
    var x0 = 440, y0 = 75, w = 210, h = 150;
    var q = [[x0, y0 + h], [x0, y0], [x0 + w, y0], [x0 + w, y0 + h]];
    line(g, q[0][0], q[0][1], q[1][0], q[1][1], { color: C.accent, w: 2.6 });
    line(g, q[0][0], q[0][1], q[2][0], q[2][1], { color: C.accent, w: 1.6, dash: '6 5' });
    line(g, q[1][0], q[1][1], q[2][0], q[2][1], { color: C.accent, w: 2.6 });
    line(g, q[2][0], q[2][1], q[3][0], q[3][1], { color: C.accent, w: 2.6 });
    line(g, q[2][0], q[2][1], q[0][0], q[0][1], { color: C.accent, w: 2.6 });
    txt(g, x0 + w * 0.6, y0 + h * 0.32, '①', { color: C.accent, size: 15, weight: 700 });
    txt(g, x0 + w * 0.32, y0 + h * 0.72, '②', { color: C.accent, size: 15, weight: 700 });
    [0, 1, 2, 3].forEach(function (i) {
      circ(g, q[i][0], q[i][1], 5, { fill: C.accent, stroke: C.bg, w: 2 });
    });
    txt(g, 555, 258, '4 个角，但 2 个角被两个三角形共用', { size: 12, color: C.muted });
    txt(g, 555, 280, '(左下、右上各属于 2 个三角形)', { size: 12, color: C.muted });
    rbox(g, 410, 292, 260, 30, { fill: C.bg2, rx: 6 });
    txt(g, 540, 312, '6 面 × 6 = 36 个渲染顶点', { size: 12.5, weight: 700, color: C.ink });
  })();

  /* ============ 10. 面法线（硬棱） vs 顶点法线（光滑） ============ */
  (function () {
    var g = root('fig-flat-smooth', 640, 340); if (!g) return;
    var R = 96, cxL = 165, cy = 175, cxR = 465;
    var LIGHT = 225 * Math.PI / 180; /* 光从左上方来 */
    title(g, 60, 34, '面法线 → 硬棱角');
    title(g, 370, 34, '顶点法线 → 光滑过渡');
    /* 左侧：8 个平面扇形，各自一个亮度 */
    for (var k = 0; k < 12; k++) {
      var a0 = (k / 12) * Math.PI * 2, a1 = ((k + 1) / 12) * Math.PI * 2;
      var mid = (a0 + a1) / 2;
      var lit = Math.max(0, Math.cos(mid - LIGHT));
      var pts = [[cxL, cy]];
      pts.push([cxL + Math.cos(a0) * R, cy + Math.sin(a0) * R]);
      pts.push([cxL + Math.cos(a1) * R, cy + Math.sin(a1) * R]);
      poly(g, pts, { fill: gray(0.16 + 0.8 * lit), stroke: C.bg, w: 1 });
    }
    circ(g, cxL, cy, R, { stroke: C.ink, w: 1.5 });
    /* 右侧：径向渐变 */
    var defs = el('defs', {});
    var lg = el('radialGradient', { id: 'smoothGrad', cx: '34%', cy: '30%', r: '78%' });
    [[0, '#f2f2f5'], [0.45, '#a8a8b0'], [1, '#3c3c44']].forEach(function (s) {
      lg.appendChild(el('stop', { offset: s[0], 'stop-color': s[1] }));
    });
    defs.appendChild(lg); g.appendChild(defs);
    circ(g, cxR, cy, R, { fill: 'url(#smoothGrad)', stroke: C.ink, w: 1.5 });
    txt(g, 165, 310, '每块三角形一个亮度，交界处是硬边', { size: 12, color: C.muted });
    txt(g, 465, 310, '法线连续变化，明暗平滑，看不出三角片', { size: 12, color: C.muted });
  })();

  /* ============ 11. 顶点法线 = 相邻面法线的平均 ============ */
  (function () {
    var g = root('fig-vertex-normal', 420, 330); if (!g) return;
    var ox = 130, oy = 250, S = 145;
    function P(u, w) { return { x: ox + u * S, y: oy - w * S }; }
    /* 两条面法线（二维示意）：朝上 + 朝右 */
    arrow(g, ox, oy, P(0, 1.02).x, P(0, 1.02).y, { color: C.accent, w: 3, head: 9 });
    txt(g, P(0, 1.06).x, P(0, 1.06).y - 4, '法线 B (0,1,0)', { color: C.accent, size: 11.5, weight: 600 });
    arrow(g, ox, oy, P(1.02, 0).x, P(1.02, 0).y, { color: C.accent, w: 3, head: 9 });
    txt(g, P(1.06, 0).x + 4, P(1.06, 0).y + 14, '法线 A (1,0,0)', { color: C.accent, size: 11.5, weight: 600, anchor: 'start' });
    /* 平均方向 (0.707, 0.707) */
    arrow(g, ox, oy, P(0.72, 0.72).x, P(0.72, 0.72).y, { color: C.accent2, w: 4, head: 11 });
    txt(g, P(0.80, 0.80).x + 8, P(0.80, 0.80).y + 4, '平均后的顶点法线', { color: C.accent2, size: 12.5, weight: 700, anchor: 'start' });
    circ(g, ox, oy, 5, { fill: C.ink });
    txt(g, ox - 10, oy + 8, '顶点', { anchor: 'end', size: 12, color: C.muted });
    rbox(g, 30, 178, 372, 44, { fill: C.bg2, stroke: C.rule });
    txt(g, 46, 197, '步骤：① 把周围所有面的法线加起来  ② 把结果归一化', { anchor: 'start', size: 12, color: C.ink });
    txt(g, 46, 215, '（本例两个面：A+B = (1,1,0) → 归一化 = (0.707,0.707,0)，正好是角平分线）', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 216, 292, '顶点挂在顶点上，被几个面共用，就取这几个面法线的平均', { size: 12.5, weight: 600, color: C.ink });
  })();

  /* ============ 12. 四种光源 ============ */
  (function () {
    var g = root('fig-light-types', 700, 520); if (!g) return;
    var cells = [
      { x: 20, y: 20, t: '点光源', sub: '有位置 · 光线从一点发散' },
      { x: 360, y: 20, t: '平行光（方向光）', sub: '无位置 · 光线互相平行' },
      { x: 20, y: 270, t: '聚光灯', sub: '有位置 · 只朝一个锥形范围' },
      { x: 360, y: 270, t: '环境光', sub: '无方向 · 四面八方均匀糊上来' }
    ];
    var W = 320, H = 220;
    cells.forEach(function (c, i) {
      rbox(g, c.x, c.y, W, H, { fill: C.bg2, stroke: C.rule });
      title(g, c.x + 18, c.y + 28, c.t);
      txt(g, c.x + 18, c.y + 48, c.sub, { anchor: 'start', size: 11.5, color: C.muted });
      var gx = c.x + W / 2, gy = c.y + 150;
      if (i === 0) {
        var sx = c.x + W / 2, sy = c.y + 78;
        circ(g, sx, sy, 6, { fill: C.accent2 });
        for (var a = 0; a < 8; a++) {
          var ang = Math.PI * 0.12 + a * (Math.PI * 0.76 / 7);
          arrow(g, sx + Math.cos(ang) * 12, sy + Math.sin(ang) * 12,
            sx + Math.cos(ang) * 62, sy + Math.sin(ang) * 62, { color: C.accent, w: 1.6, head: 6 });
        }
        line(g, c.x + 40, c.y + 172, c.x + W - 40, c.y + 172, { color: C.ink, w: 3 });
      } else if (i === 1) {
        for (var k = 0; k < 5; k++) {
          var px = c.x + 60 + k * 50;
          arrow(g, px, c.y + 80, px, c.y + 160, { color: C.accent, w: 1.8, head: 7 });
        }
        line(g, c.x + 40, c.y + 172, c.x + W - 40, c.y + 172, { color: C.ink, w: 3 });
      } else if (i === 2) {
        var qx = c.x + W / 2, qy = c.y + 78;
        circ(g, qx, qy, 6, { fill: C.accent2 });
        [-1, 1].forEach(function (sgn) {
          var ang2 = sgn * 0.42 + Math.PI / 2;
          arrow(g, qx + Math.cos(ang2) * 12, qy + Math.sin(ang2) * 12,
            qx + Math.cos(ang2) * 88, qy + Math.sin(ang2) * 88, { color: C.accent, w: 1.8, head: 7 });
        });
        arrow(g, qx, qy + 10, qx, qy + 96, { color: C.accent, w: 1.8, head: 7 });
        line(g, c.x + 40, c.y + 172, c.x + W - 40, c.y + 172, { color: C.ink, w: 3 });
      } else {
        circ(g, c.x + W / 2, c.y + 140, 26, { fill: C.bg, stroke: C.ink, w: 2 });
        for (var j = 0; j < 8; j++) {
          var aa = j * Math.PI / 4;
          var bx = c.x + W / 2 + Math.cos(aa) * 54, by = c.y + 140 + Math.sin(aa) * 54;
          var ex = c.x + W / 2 + Math.cos(aa) * 30, ey = c.y + 140 + Math.sin(aa) * 30;
          arrow(g, bx, by, ex, ey, { color: C.accent, w: 1.6, head: 6 });
        }
      }
    });
    txt(g, 350, 508, '我们项目用的是【点光源】：把光源存成一个点，每个顶点各自算「指向它」的方向', { size: 12.5, weight: 600, color: C.ink });
  })();

  /* ============ 13. 手电筒三组实验 ============ */
  (function () {
    var g = root('fig-flashlight', 700, 300); if (!g) return;
    var items = [
      { t: 'A. 正对着照', sub: '光斑小、最亮', type: 'front' },
      { t: 'B. 斜着照', sub: '光斑被拉长、变暗', type: 'tilt' },
      { t: 'C. 几乎平行照', sub: '几乎照不到、不亮', type: 'para' }
    ];
    items.forEach(function (it, i) {
      var x0 = 20 + i * 226, w = 200, y0 = 40;
      rbox(g, x0, y0, w, 200, { fill: C.bg2, stroke: C.rule });
      title(g, x0 + 14, y0 + 24, it.t);
      var wallY = y0 + 150;
      line(g, x0 + 24, wallY, x0 + w - 24, wallY, { color: C.ink, w: 3.5 });
      var cx = x0 + w / 2;
      if (it.type === 'front') {
        arrow(g, cx, y0 + 46, cx, wallY - 10, { color: C.accent, w: 2.4, head: 8 });
        g.appendChild(el('ellipse', { cx: cx, cy: wallY, rx: 22, ry: 9, fill: C.accent2, 'fill-opacity': '0.95' }));
      } else if (it.type === 'tilt') {
        arrow(g, cx + 48, y0 + 46, cx + 6, wallY - 10, { color: C.accent, w: 2.4, head: 8 });
        g.appendChild(el('ellipse', { cx: cx, cy: wallY, rx: 62, ry: 11, fill: C.accent2, 'fill-opacity': '0.42' }));
      } else {
        arrow(g, x0 + 24, y0 + 96, x0 + w - 24, y0 + 100, { color: C.accent, w: 2.4, head: 8 });
        txt(g, cx, wallY + 26, '几乎没有光斑', { size: 11.5, color: C.muted });
      }
      txt(g, x0 + w / 2, y0 + 178, it.sub, { size: 12, weight: 600, color: C.ink });
    });
    txt(g, 350, 274, '同一个手电筒、同样多的光 —— 差别只在于「被摊到了多大面积上」', { size: 12.5, color: C.muted });
  })();

  /* ============ 14. 点积的三种情况 ============ */
  (function () {
    var g = root('fig-dot-product', 680, 250); if (!g) return;
    var cases = [
      { t: '完全同向', ang: 0, val: '1', note: '夹角 0°', col: C.accent },
      { t: '互相垂直', ang: 90, val: '0', note: '夹角 90°', col: C.accent2 },
      { t: '完全相反', ang: 180, val: '-1', note: '夹角 180°', col: C.muted }
    ];
    cases.forEach(function (c, i) {
      var x0 = 20 + i * 220, w = 200;
      rbox(g, x0, 20, w, 205, { fill: C.bg2, stroke: C.rule });
      title(g, x0 + w / 2, 46, c.t);
      var ox = x0 + w / 2, oy = 160, L = 62;
      arrow(g, ox, oy, ox + L, oy, { color: C.ink, w: 3, head: 9 });
      var rad = c.ang * Math.PI / 180;
      arrow(g, ox, oy, ox + Math.cos(rad) * L, oy - Math.sin(rad) * L, { color: c.col, w: 3, head: 9 });
      txt(g, ox + 12, oy + 16, 'a', { anchor: 'start', size: 12, color: C.ink, weight: 600 });
      txt(g, ox + Math.cos(rad) * (L + 14) + 2, oy - Math.sin(rad) * (L + 14) - 2, 'b', { size: 12, color: c.col, weight: 600 });
      var bx = x0 + w / 2;
      rbox(g, bx - 44, 176, 88, 34, { fill: C.bg, stroke: c.col, rx: 8 });
      txt(g, bx, 198, '点积 = ' + c.val, { size: 14, weight: 700, color: c.col });
      txt(g, bx, 88, c.note, { size: 12, color: C.muted });
    });
    txt(g, 340, 243, '单位向量的点积 = 「两个方向有多同向」的分数', { size: 12.5, weight: 600, color: C.ink });
  })();

  /* ============ 15. ambient 高低对比 ============ */
  (function () {
    var g = root('fig-ambient', 680, 320); if (!g) return;
    function row(y, amb, label, note) {
      title(g, 24, y - 12, label);
      var vals = [1, 0.5, 0];
      var names = ['正受光', '斜 45°', '背光'];
      vals.forEach(function (d, i) {
        var it = amb + (1 - amb) * d;
        var x = 40 + i * 130;
        g.appendChild(el('rect', { x: x, y: y, width: 100, height: 74, rx: 8, fill: gray(it), stroke: C.rule }));
        txt(g, x + 50, y + 96, names[i], { size: 12, color: C.muted });
        txt(g, x + 50, y + 40, it.toFixed(2), { size: 15, weight: 700, color: it > 0.6 ? '#22242a' : '#f2f2f5' });
      });
      txt(g, 470, y + 34, note[0], { anchor: 'start', size: 12, color: C.ink });
      txt(g, 470, y + 54, note[1], { anchor: 'start', size: 12, color: C.muted });
    }
    row(55, 0.2, 'ambient = 0.2', ['三块差异 1.00 → 0.20', '对比强烈，棱角清晰']);
    row(205, 0.6, 'ambient = 0.6', ['三块差异 1.00 → 0.60', '对比被压缩，发平、发糊']);
    txt(g, 340, 308, 'intensity = ambient + (1 − ambient) × diffuse', { size: 12.5, weight: 600, color: C.accent });
  })();

  /* ============ 16. 结论：光方向必须是 顶点 → 光源 ============ */
  (function () {
    var g = root('fig-L-dir', 620, 260); if (!g) return;
    title(g, 30, 34, '✅ 正确：L = 顶点 → 光源');
    title(g, 340, 34, '❌ 写反：L = 光源 → 顶点');
    function panel(x0, ok) {
      var y0 = 55, w = 250, h = 150;
      rbox(g, x0, y0, w, h, { fill: C.bg2, stroke: ok ? C.accent : C.accent2 });
      var vx = x0 + 55, vy = y0 + 118;      /* 顶点 */
      var lx = x0 + 195, ly = y0 + 42;      /* 光源 */
      circ(g, lx, ly, 7, { fill: C.accent2 });
      txt(g, lx, ly - 12, '光源', { size: 11.5, color: C.accent2, weight: 600 });
      circ(g, vx, vy, 6, { fill: C.ink });
      txt(g, vx, vy + 20, '顶点', { size: 11.5, color: C.muted });
      if (ok) arrow(g, vx, vy, lx - 10, ly + 10, { color: C.accent, w: 3, head: 9 });
      else arrow(g, lx, ly, vx + 9, vy - 9, { color: C.accent2, w: 3, head: 9 });
      txt(g, x0 + w / 2, y0 + h - 10, ok ? 'dot(N, L) 为正 → 面被照亮' : 'dot 全反号 → 亮暗颠倒', { size: 11.5, color: ok ? C.accent : C.accent2, weight: 600 });
    }
    panel(30, true);
    panel(340, false);
  })();

  /* ---------- 侧边导航：滚动高亮当前章节 ---------- */
  var links = Array.prototype.slice.call(document.querySelectorAll('.nav a'));
  var targets = links.map(function (a) {
    var href = a.getAttribute('href') || '';
    return document.getElementById(href.slice(1));
  }).filter(Boolean);
  if (targets.length) {
    var spy = function () {
      var pos = window.scrollY + window.innerHeight * 0.28;
      var active = targets[0];
      targets.forEach(function (t) { if (t.offsetTop <= pos) active = t; });
      links.forEach(function (a) {
        a.classList.toggle('active', a.getAttribute('href') === '#' + active.id);
      });
    };
    window.addEventListener('scroll', spy, { passive: true });
    window.addEventListener('resize', spy);
    spy();
  }

  /* 全部画完后标记，方便调试与验证 */
  document.documentElement.setAttribute('data-figs', 'ok');
})();