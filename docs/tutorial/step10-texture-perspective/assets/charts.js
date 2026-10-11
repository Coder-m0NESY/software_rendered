/* 第 10 步教程 · 插图库
   全部用内联 SVG 手绘，不依赖任何第三方库。
   这些是「几何原理图」，不是数据图表 —— 需要精确标注的 UV 坐标系、
   透视投影关系和数值刻度，所以直接用 SVG 画。 */
(function () {
  var NS = 'http://www.w3.org/2000/svg';
  var cs = getComputedStyle(document.documentElement);
  function V(n) { return cs.getPropertyValue(n).trim(); }
  var C = {
    ink: V('--ink'), muted: V('--muted'), rule: V('--rule'),
    accent: V('--accent'), accent2: V('--accent2'),
    bg: V('--bg'), bg2: V('--bg2'),
    tileA: V('--tile-a'), tileB: V('--tile-b')
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
  function title(g, x, y, s, o) {
    txt(g, x, y, s, { anchor: 'start', size: 12.5, weight: 600, color: (o && o.color) || C.accent2 });
  }

  /* 在矩形里画 n×n 的棋盘格（模拟一张贴图） */
  function checker(g, x, y, size, n, ca, cb) {
    var cell = size / n;
    for (var i = 0; i < n; ++i) {
      for (var j = 0; j < n; ++j) {
        g.appendChild(el('rect', {
          x: x + i * cell, y: y + j * cell, width: cell, height: cell,
          fill: ((i + j) % 2 === 0) ? ca : cb, stroke: 'none'
        }));
      }
    }
  }
  /* 六位十六进制颜色 → {r,g,b}；也接受 rgb(...) */
  function parseColor(s) {
    s = (s || '').trim();
    var m = s.match(/^#([0-9a-f]{6})$/i);
    if (m) {
      var v = parseInt(m[1], 16);
      return { r: (v >> 16) & 255, g: (v >> 8) & 255, b: v & 255 };
    }
    m = s.match(/rgba?\(([^)]+)\)/);
    if (m) {
      var p = m[1].split(',').map(function (t) { return parseFloat(t); });
      return { r: p[0], g: p[1], b: p[2] };
    }
    return { r: 128, g: 128, b: 128 };
  }
  function mix(a, b, t) {
    var A = parseColor(a), B = parseColor(b);
    return 'rgb(' + Math.round(A.r + (B.r - A.r) * t) + ',' +
                    Math.round(A.g + (B.g - A.g) * t) + ',' +
                    Math.round(A.b + (B.b - A.b) * t) + ')';
  }
  /* 四角用不同颜色，方便和「贴到面上」的那张图一一对应 */
  var CORNER = ['#2f5bea', '#d9741a', '#39a24b', '#b455c9'];

  /* ============ 图 1：UV 坐标系 ============ */
  (function () {
    var g = root('fig-uv-space', 440, 340); if (!g) return;
    var x0 = 130, y0 = 60, S = 180;
    title(g, 130, 34, 'UV 坐标系（本项目约定：原点在左上角）');

    checker(g, x0, y0, S, 4, C.tileA, C.tileB);
    g.appendChild(el('rect', {
      x: x0, y: y0, width: S, height: S, fill: 'none', stroke: C.ink, 'stroke-width': 1.5
    }));

    /* 四个角 + uv 标签 */
    var corners = [
      [x0,     y0,     '(0, 0)', 'end',  -8,  -8],
      [x0 + S, y0,     '(1, 0)', 'start', 8,  -8],
      [x0,     y0 + S, '(0, 1)', 'end',  -8,  16],
      [x0 + S, y0 + S, '(1, 1)', 'start', 8,  16]
    ];
    corners.forEach(function (c) {
      circ(g, c[0], c[1], 5, { fill: C.accent, stroke: C.bg, w: 2 });
      txt(g, c[0] + c[4], c[1] + c[5], c[2], { anchor: c[3], size: 12.5, weight: 700, color: C.accent });
    });

    /* u 轴（向右） */
    arrow(g, x0, y0 + S + 30, x0 + S + 46, y0 + S + 30, { color: C.muted, w: 1.8 });
    line(g, x0, y0 + S + 24, x0, y0 + S + 36, { color: C.muted, w: 1.5 });
    line(g, x0 + S, y0 + S + 24, x0 + S, y0 + S + 36, { color: C.muted, w: 1.5 });
    txt(g, x0, y0 + S + 52, 'u = 0', { size: 11.5, color: C.muted });
    txt(g, x0 + S, y0 + S + 52, 'u = 1', { size: 11.5, color: C.muted });
    txt(g, x0 + S + 56, y0 + S + 34, 'u（右）', { anchor: 'start', size: 11.5, color: C.muted });

    /* v 轴（向下） */
    arrow(g, x0 - 34, y0, x0 - 34, y0 + S + 46, { color: C.muted, w: 1.8 });
    line(g, x0 - 40, y0, x0 - 28, y0, { color: C.muted, w: 1.5 });
    line(g, x0 - 40, y0 + S, x0 - 28, y0 + S, { color: C.muted, w: 1.5 });
    txt(g, x0 - 40, y0 + 4, 'v = 0', { anchor: 'end', size: 11.5, color: C.muted });
    txt(g, x0 - 40, y0 + S + 4, 'v = 1', { anchor: 'end', size: 11.5, color: C.muted });
    txt(g, x0 - 44, y0 + S + 50, 'v（下）', { anchor: 'end', size: 11.5, color: C.muted });
  })();

  /* ============ 图 2：uv 超出 [0,1] → 平铺 ============ */
  (function () {
    var g = root('fig-tile', 540, 350); if (!g) return;
    var tw = 110, cols = [130, 240, 350], rows = [70, 180];
    title(g, 130, 44, 'uv 超出 0~1：重复平铺（tiling）');

    var colLab = ['u ∈ [-1, 0]', 'u ∈ [0, 1]', 'u ∈ [1, 2]'];
    var rowLab = ['v ∈ [-1, 0]', 'v ∈ [0, 1]'];

    rows.forEach(function (ry, r) {
      cols.forEach(function (cx, c) {
        checker(g, cx, ry, tw, 4, C.tileA, C.tileB);
        var hl = (c === 1 && r === 1);
        g.appendChild(el('rect', {
          x: cx, y: ry, width: tw, height: tw, fill: 'none',
          stroke: hl ? C.accent : C.rule, 'stroke-width': hl ? 2.5 : 1
        }));
      });
      txt(g, 122, ry + tw / 2 + 4, rowLab[r], { anchor: 'end', size: 11, color: C.muted });
    });
    cols.forEach(function (cx, c) {
      txt(g, cx + tw / 2, 62, colLab[c], { size: 11.5, color: C.muted, weight: c === 1 ? 700 : 400 });
    });

    /* 高亮那一块是棋盘格，直接压字会看不清，先垫一个白底 */
    rbox(g, 240 + tw / 2 - 46, 180 + tw / 2 - 13, 92, 27, { fill: C.bg, stroke: C.accent, rx: 7, w: 1.5 });
    txt(g, 240 + tw / 2, 180 + tw / 2 + 5, '一张贴图', { size: 12.5, weight: 700, color: C.accent });
    txt(g, 270, 320, '带蓝框的那一块 = 一张贴图（u、v 都落在 [0, 1]），周围是同一张图又铺了一遍', { size: 11.5, color: C.ink });
    txt(g, 270, 340, 'u = -1.2 会绕到 u = 0.8 的位置 —— 靠 ((v % n) + n) % n 把负数掰回正数', { size: 11.5, color: C.muted });
  })();

  /* ============ 图 3：贴图 ↔ 立方体一个面 ============ */
  (function () {
    var g = root('fig-uv-face', 640, 330); if (!g) return;
    var x0 = 45, y0 = 80, S = 160;
    title(g, 45, 40, '把一张贴图贴到立方体的一个面上');
    txt(g, 125, 62, '贴图（UV 空间）', { size: 12, weight: 600, color: C.muted });
    txt(g, 470, 62, '立方体的一个面', { size: 12, weight: 600, color: C.muted });
    txt(g, 470, 78, '（三维里的一个平面）', { size: 11, color: C.muted });

    checker(g, x0, y0, S, 4, C.tileA, C.tileB);
    g.appendChild(el('rect', { x: x0, y: y0, width: S, height: S, fill: 'none', stroke: C.ink, 'stroke-width': 1.5 }));

    /* 面上四点（顺序：左上 右上 左下 右下） */
    var A = [400, 70], B = [560, 105], D = [375, 230], E = [535, 265];
    poly(g, [A, B, E, D], { fill: 'rgba(47,91,234,.07)', stroke: C.ink, w: 2.5 });

    var src = [[x0, y0], [x0 + S, y0], [x0, y0 + S], [x0 + S, y0 + S]];
    var dst = [A, B, D, E];
    var lab = ['(0, 0)', '(1, 0)', '(0, 1)', '(1, 1)'];
    var anchor = ['end', 'start', 'end', 'start'];

    for (var i = 0; i < 4; ++i) {
      line(g, src[i][0], src[i][1], dst[i][0], dst[i][1], { color: C.rule, w: 1.4, dash: '5 4' });
      circ(g, src[i][0], src[i][1], 5.5, { fill: CORNER[i], stroke: C.bg, w: 2 });
      circ(g, dst[i][0], dst[i][1], 5.5, { fill: CORNER[i], stroke: C.bg, w: 2 });
      txt(g, src[i][0] + (i % 2 === 0 ? -10 : 10), src[i][1] + (i < 2 ? -10 : 18), lab[i],
          { anchor: anchor[i], size: 11.5, weight: 700, color: CORNER[i] });
      txt(g, dst[i][0] + (i % 2 === 0 ? -12 : 12), dst[i][1] + (i < 2 ? -10 : 20), lab[i],
          { anchor: anchor[i], size: 11.5, weight: 700, color: CORNER[i] });
    }

    txt(g, 320, 305, '四个角的 uv 一一对应 —— 虚线就是这种对应关系。', { size: 12, color: C.muted });
  })();

  /* ============ 图 4：最近邻 vs 双线性 ============ */
  (function () {
    var g = root('fig-sample', 600, 310); if (!g) return;
    var pw = 240, ph = 150, y0 = 80;
    var panels = [[45, '最近邻（nearest）'], [315, '双线性（bilinear）']];
    title(g, 45, 44, '把「两个像素宽的边界」放大 10 倍');

    panels.forEach(function (p, idx) {
      var x0 = p[0];
      txt(g, x0 + pw / 2, 66, p[1], { size: 12.5, weight: 700, color: idx === 0 ? C.accent2 : C.accent });
      var band = pw / 10;
      for (var i = 0; i < 10; ++i) {
        var fill;
        if (idx === 0) {
          fill = i < 5 ? C.tileB : C.tileA;                 // 硬跳变
        } else {
          fill = mix(C.tileB, C.tileA, Math.min(1, Math.max(0, (i - 4.5 + 1) / 2)));  // 平滑过渡
        }
        g.appendChild(el('rect', {
          x: x0 + i * band, y: y0, width: band + 0.6, height: ph, fill: fill, stroke: 'none'
        }));
      }
      /* 像素之间的分界线（示意「这是放大后的像素」） */
      for (var k = 0; k <= 10; ++k) {
        line(g, x0 + k * band, y0, x0 + k * band, y0 + ph, { color: 'rgba(0,0,0,.10)', w: 1 });
      }
      g.appendChild(el('rect', { x: x0, y: y0, width: pw, height: ph, fill: 'none', stroke: C.rule, 'stroke-width': 1.5 }));
      txt(g, x0 + 5 * band, y0 + ph + 20, '边界', { size: 11.5, color: C.muted });
      line(g, x0 + 5 * band, y0 + ph + 4, x0 + 5 * band, y0 + ph + 12, { color: C.muted, w: 1.2 });
    });

    txt(g, 300, 292, '左边：5 个深色像素 + 5 个浅色像素 = 一条台阶。右边：同样 10 个像素，但颜色是渐变的。',
        { size: 12, color: C.muted });
  })();

  /* ============ 图 5（核心）：屏幕上的一半 ≠ 世界里的一半 ============ */
  (function () {
    var g = root('fig-persp', 900, 418); if (!g) return;
    title(g, 40, 36, '地板的侧视图：近端 z = 2，远端 z = 10');
    txt(g, 40, 56, '梯形 = 以及它投影到屏幕上的样子。横线是「按屏幕高度等分」画出来的。',
        { anchor: 'start', size: 11.5, color: C.muted });

    var yNear = 330, yFar = 110;              // 近端在下方、远端在上方
    var wNear = 480, wFar = 140;
    var cxNear = 360, cxFar = 360;            // 视平线竖直方向居中

    function yOf(yn) { return yNear + (yFar - yNear) * yn; }
    function xOf(yn) { return [cxNear + (cxFar - cxNear) * yn - (wNear + (wFar - wNear) * yn) / 2,
                               cxNear + (cxFar - cxNear) * yn + (wNear + (wFar - wNear) * yn) / 2]; }

    /* 梯形本体 */
    var b = xOf(0), t = xOf(1);
    poly(g, [[b[0], yOf(0)], [b[1], yOf(0)], [t[1], yOf(1)], [t[0], yOf(1)]],
         { fill: 'rgba(47,91,234,.07)', stroke: C.ink, w: 2.5 });

    /* 贴地相机 */
    var camX = 100, camY = 374;
    circ(g, camX, camY, 6, { fill: C.ink });
    line(g, camX, camY, b[0], yOf(0), { color: C.rule, w: 1.4, dash: '5 4' });
    line(g, camX, camY, b[1], yOf(0), { color: C.rule, w: 1.4, dash: '5 4' });
    txt(g, camX + 14, camY + 5, '相机（贴地看）', { anchor: 'start', size: 11.5, color: C.muted });

    /* 四条「按屏幕高度等分」的横线 */
    var marks = [
      { yn: 0,      color: C.ink,     w: 2,   dash: 'none', label: 't = 0　近端（z = 2）',           lc: C.ink,     bold: false },
      { yn: 1 / 6,  color: C.accent,  w: 3,   dash: 'none', label: '屏幕正中间　→　t = 1/6',        lc: C.accent,  bold: true  },
      { yn: 0.5,    color: C.ink,     w: 1,   dash: '6 4',  label: '（真正的屏幕中点在这里，t = 1/6）', lc: C.muted,   bold: false },
      { yn: 0.8333, color: C.accent2, w: 2.5, dash: '7 4',  label: '世界的正中间　t = 1/2',         lc: C.accent2, bold: true  },
      { yn: 1,      color: C.ink,     w: 2,   dash: 'none', label: 't = 1　远端（z = 10）',          lc: C.ink,     bold: false }
    ];
    marks.forEach(function (m) {
      var y = yOf(m.yn), xr = xOf(m.yn);
      line(g, xr[0] - 22, y, xr[1] + 22, y, { color: m.color, w: m.w, dash: m.dash });
      txt(g, 640, y + 4, m.label, { anchor: 'start', size: m.bold ? 12.5 : 12, weight: m.bold ? 700 : 400, color: m.lc });
    });

    /* 右侧说明（放在最上面一行，避开 t = 1 那条横线的标注） */
    txt(g, 640, 64, '横线的 y 位置 = 屏幕上的位置', { anchor: 'start', size: 11, color: C.muted });
    txt(g, 640, 78, '文字里的 t = 世界空间的位置', { anchor: 'start', size: 11, color: C.muted });

    txt(g, 40, 404, '屏幕的中点（y 位置在正中间）对应世界的 1/6 处；世界的 1/2 处已经跑到了屏幕上偏上的位置。差 3 倍。',
        { anchor: 'start', size: 12.5, weight: 700, color: C.accent2 });
  })();

  /* ============ 图 6：透视校正的数据流 ============ */
  (function () {
    var g = root('fig-correct', 860, 250); if (!g) return;
    title(g, 30, 40, '透视校正的数据流');
    var y = 70, h = 96, w = 176;
    var xs = [30, 240, 450, 660];
    var boxes = [
      ['顶点阶段', ['uv、w 已知', '算出 invw = 1/w'], C.accent],
      ['屏幕空间插值', ['u/w、v/w、1/w', '（三者都线性，合法）'], C.accent],
      ['相除', ['u = interp(u/w)', '／ interp(1/w)'], C.accent2],
      ['采样贴图', ['uv → 像素下标', '→ 颜色'], C.accent]
    ];
    boxes.forEach(function (bx, i) {
      rbox(g, xs[i], y, w, h, { fill: C.bg2, stroke: C.rule });
      g.appendChild(el('rect', { x: xs[i], y: y, width: 4, height: h, rx: 2, fill: bx[2], stroke: 'none' }));
      txt(g, xs[i] + w / 2 + 2, y + 28, bx[0], { size: 13, weight: 700, color: bx[2] });
      txt(g, xs[i] + w / 2 + 2, y + 52, bx[1][0], { size: 11.5, color: C.muted });
      txt(g, xs[i] + w / 2 + 2, y + 70, bx[1][1], { size: 11.5, color: C.muted });
    });
    for (var i = 0; i < 3; ++i) {
      arrow(g, xs[i] + w + 6, y + h / 2, xs[i + 1] - 8, y + h / 2, { color: C.muted, w: 2 });
    }
    txt(g, 30, 210, '① 先除以 w 再插值　② 插值完再除回来 —— 顺序不能反', { anchor: 'start', size: 12.5, weight: 700, color: C.accent2 });
    txt(g, 30, 232, '代价：每像素两次乘加 + 一次除法。', { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 7：地板尺寸的定量约束（俯视图） ============ */
  (function () {
    var g = root('fig-floor', 500, 390); if (!g) return;
    var ox = 250, oy = 190, SC = 30;     // 30 px / 世界单位
    title(g, 40, 40, '俯视图：地板尺寸为什么不能随手定');

    /* 相机轨道 r = 5 */
    circ(g, ox, oy, 5 * SC, { stroke: C.accent, w: 2 });
    /* 地板 6×6（半边长 3） */
    g.appendChild(el('rect', {
      x: ox - 3 * SC, y: oy - 3 * SC, width: 6 * SC, height: 6 * SC,
      fill: 'rgba(57,162,75,.14)', stroke: '#39a24b', 'stroke-width': 2.5
    }));

    /* 坐标轴提示 */
    arrow(g, ox, oy, ox + 4.4 * SC, oy, { color: C.rule, w: 1.6 });
    arrow(g, ox, oy, ox, oy + 4.4 * SC, { color: C.rule, w: 1.6 });
    txt(g, ox + 4.4 * SC + 12, oy + 4, 'x', { size: 11.5, color: C.muted });
    txt(g, ox + 6, oy + 4.4 * SC + 14, 'z', { size: 11.5, color: C.muted });

    /* 中心 + 最远的角 */
    circ(g, ox, oy, 4, { fill: C.ink });
    var cx = ox + 3 * SC, cy = oy + 3 * SC;
    line(g, ox, oy, cx, cy, { color: C.accent2, w: 2.5 });
    circ(g, cx, cy, 5.5, { fill: C.accent2, stroke: C.bg, w: 2 });
    txt(g, 300, 258, '水平距离 √(3²+3²) ≈ 4.24', { anchor: 'start', size: 12, weight: 700, color: C.accent2 });
    txt(g, 300, 276, '再加地板的高度 y = -1 → 4.36', { anchor: 'start', size: 11.5, color: C.muted });

    txt(g, ox, oy - 3 * SC - 14, '地板 6×6（半边长 3）', { size: 12, weight: 600, color: '#39a24b' });
    txt(g, 120, 62, '相机活动轨道 r = 5', { anchor: 'start', size: 12, weight: 600, color: C.accent });

    txt(g, 250, 356, '4.36 < 5 ✓', { size: 15, weight: 800, color: C.accent });
    txt(g, 250, 378, '地板的角必须落在相机轨道里面，否则那个角会跑到相机背后，整块地板被丢掉（还没有裁剪）',
        { size: 11.5, color: C.muted });
  })();

  /* ---------- 侧边导航高亮 ---------- */
  (function () {
    var links = Array.prototype.slice.call(document.querySelectorAll('aside.nav a'));
    if (!links.length || !('IntersectionObserver' in window)) return;
    var map = {};
    links.forEach(function (a) { map[a.getAttribute('href').slice(1)] = a; });
    var obs = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) {
        if (!e.isIntersecting) return;
        links.forEach(function (a) { a.classList.remove('active'); });
        var a = map[e.target.id];
        if (a) a.classList.add('active');
      });
    }, { rootMargin: '-20% 0px -70% 0px', threshold: 0 });
    Object.keys(map).forEach(function (id) {
      var s = document.getElementById(id);
      if (s) obs.observe(s);
    });
  })();
})();