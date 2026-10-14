/* 第 12 步教程 · 插图库
   全部用内联 SVG 手绘，不依赖任何第三方库。
   这些是「几何 / 数据流原理图」，不是数据图表 —— 需要精确标注的坐标、
   表格结构和下标对应关系，所以直接用 SVG 画。
   设计系统与第 9、10、11 步教程保持一致（同一套 CSS 变量）。 */
(function () {
  var NS = 'http://www.w3.org/2000/svg';
  var cs = getComputedStyle(document.documentElement);
  function V(n) { return cs.getPropertyValue(n).trim(); }
  var C = {
    ink: V('--ink'), muted: V('--muted'), rule: V('--rule'),
    accent: V('--accent'), accent2: V('--accent2'),
    bg: V('--bg'), bg2: V('--bg2')
  };
  var GREEN = '#39a24b';
  var PURPLE = '#b455c9';

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
  function lerp(a, b, t) { return [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t]; }

  /* ============ 图 1：OBJ 的三条索引流 ============ */
  (function () {
    var g = root('fig-streams', 900, 352); if (!g) return;
    title(g, 30, 32, 'OBJ 的三张表：一个角只是一组「三元组下标」');

    var cols = [
      { x: 40,  w: 130, name: 'v',  color: C.accent,  hi: 5, fmt: function (k) { return 'v' + k; } },
      { x: 190, w: 130, name: 'vt', color: C.accent2, hi: 2, fmt: function (k) { return 'vt' + k; } },
      { x: 340, w: 130, name: 'vn', color: GREEN,     hi: 3, fmt: function (k) { return 'vn' + k; } }
    ];
    var top = 74, rh = 30, rows = 6;

    cols.forEach(function (c) {
      rbox(g, c.x, top, c.w, 26 + rows * rh, { rx: 8, fill: C.bg2, stroke: C.rule });
      txt(g, c.x + c.w / 2, top + 17, c.name + '[]', { size: 12.5, weight: 700, color: c.color });
      for (var k = 1; k <= rows; k++) {
        var cy = top + 26 + (k - 1) * rh + rh / 2;
        var on = (k === c.hi);
        if (on) {
          g.appendChild(el('rect', {
            x: c.x + 1, y: top + 26 + (k - 1) * rh, width: c.w - 2, height: rh,
            fill: 'rgba(47,91,234,.10)', stroke: 'none'
          }));
        }
        txt(g, c.x + c.w / 2, cy + 4, c.fmt(k), {
          size: 12, color: on ? c.color : C.muted, weight: on ? 700 : 400
        });
      }
      txt(g, c.x + c.w / 2, top + 26 + rows * rh + 34, '索引 ' + c.hi, { size: 11, color: c.color, weight: 700 });
    });

    /* 面：一个角的三个数字，分别连到三张表 */
    var fx = 620, fy = 108, fw = 246, fh = 150;
    rbox(g, fx, fy, fw, fh, { rx: 10, fill: 'rgba(217,116,26,.06)', stroke: C.rule });
    txt(g, fx + fw / 2, fy + 26, 'f  5/2/3', { size: 18, weight: 700, color: C.ink });
    txt(g, fx + fw / 2, fy + 52, '（一个角 = 一个三元组）', { size: 11.5, color: C.muted });

    var vRowY  = top + 26 + (5 - 1) * rh + rh / 2;
    var vtRowY = top + 26 + (2 - 1) * rh + rh / 2;
    var vnRowY = top + 26 + (3 - 1) * rh + rh / 2;
    line(g, 470, vRowY,  fx, 134, { color: C.accent,  w: 1.6, dash: '5 4' });
    line(g, 470, vtRowY, fx, 158, { color: C.accent2, w: 1.6, dash: '5 4' });
    line(g, 470, vnRowY, fx, 182, { color: GREEN,     w: 1.6, dash: '5 4' });

    txt(g, 490, 120, '位置', { anchor: 'start', size: 11, color: C.accent });
    txt(g, 490, 196, 'UV',   { anchor: 'start', size: 11, color: C.accent2 });
    txt(g, 490, 276, '法线', { anchor: 'start', size: 11, color: GREEN });

    txt(g, 30, 340, '三个数字之间没有任何对应关系 —— 它们只是各查各的表。所以后面必须做「图元装配」，把三张表合成一张。',
        { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 2：多边形扇形三角化 ============ */
  (function () {
    var g = root('fig-fan', 640, 300); if (!g) return;
    title(g, 40, 32, '五边形 → 扇形三角化 → 3 个三角形');

    var cx = 250, cy = 168, R = 108;
    var pts = [];
    for (var k = 0; k < 5; k++) {
      var a = -Math.PI / 2 + k * 2 * Math.PI / 5;
      pts.push([cx + R * Math.cos(a), cy + R * Math.sin(a)]);
    }
    var fills = ['rgba(47,91,234,.13)', 'rgba(57,162,75,.16)', 'rgba(217,116,26,.13)'];
    var strokes = [C.accent, GREEN, C.accent2];
    for (var i = 1; i + 1 < pts.length; i++) {
      poly(g, [pts[0], pts[i], pts[i + 1]], { fill: fills[i - 1], stroke: strokes[i - 1], w: 2 });
    }
    for (var j = 0; j < pts.length; j++) {
      circ(g, pts[j][0], pts[j][1], 5.5, { fill: C.ink, stroke: C.bg, w: 2 });
      var lx = pts[j][0] + (pts[j][0] >= cx ? 15 : -15);
      txt(g, lx, pts[j][1] + (j === 0 ? -6 : 5), 'p' + j, { size: 12, weight: 700, color: C.ink });
    }
    txt(g, cx, cy + 5, '扇心', { size: 11, color: C.muted });

    txt(g, 420, 130, '三角形 1： (p0, p1, p2)', { anchor: 'start', size: 13, weight: 700, color: C.accent });
    txt(g, 420, 162, '三角形 2： (p0, p2, p3)', { anchor: 'start', size: 13, weight: 700, color: GREEN });
    txt(g, 420, 194, '三角形 3： (p0, p3, p4)', { anchor: 'start', size: 13, weight: 700, color: C.accent2 });

    txt(g, 40, 288, '固定拿 p0 当扇心，n 边形成 n−2 个三角形。对凸多边形一定正确。',
        { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 3：图元装配（一个角点展开成三个顶点）============ */
  (function () {
    var g = root('fig-deindex', 660, 330); if (!g) return;
    title(g, 40, 32, '立方体一个角点 (1,1,1)：位置相同，属性不同 → 三个顶点');

    /* 三个面示意（用三个平行四边形代表 +X / +Y / +Z 面） */
    var bx = 60, by = 96;
    function face(dx, dy, col, lab) {
      poly(g, [[bx + dx, by + dy], [bx + dx + 120, by + dy - 34],
               [bx + dx + 120, by + dy + 46], [bx + dx, by + dy + 80]],
           { fill: col, stroke: C.rule, w: 1.5 });
      txt(g, bx + dx + 60, by + dy + 24, lab, { size: 12.5, weight: 700, color: C.ink });
      circ(g, bx + dx + 120, by + dy - 12, 5, { fill: C.ink });
    }
    face(-8, 6, 'rgba(47,91,234,.12)', '+X 面');
    face(52, 52, 'rgba(57,162,75,.14)', '+Y 面');
    face(112, 98, 'rgba(217,116,26,.13)', '+Z 面');
    txt(g, 60 + 112 + 120 + 12, 96 + 98 - 12 - 6, '共享同一个位置', { anchor: 'start', size: 11, color: C.muted });

    /* 右侧三个顶点卡 */
    var vx = 420, vy = 90, vh = 62, vw = 210;
    var data = [
      ['顶点 A', 'pos(1,1,1)  法线(1,0,0)  uv(0,0)', C.accent],
      ['顶点 B', 'pos(1,1,1)  法线(0,1,0)  uv(0,0)', GREEN],
      ['顶点 C', 'pos(1,1,1)  法线(0,0,1)  uv(0,0)', C.accent2]
    ];
    data.forEach(function (d, i) {
      var y = vy + i * (vh + 14);
      rbox(g, vx, y, vw, vh, { rx: 8, fill: C.bg2, stroke: d[2], w: 1.6 });
      txt(g, vx + 14, y + 24, d[0], { anchor: 'start', size: 13, weight: 700, color: d[2] });
      txt(g, vx + 14, y + 46, d[1], { anchor: 'start', size: 11.5, color: C.muted });
      arrow(g, 300, 172, vx - 8, y + vh / 2, { color: d[2], w: 1.8, head: 7, dash: '5 4' });
    });

    txt(g, 40, 314, '8 个位置 → 每个位置被 3 个面共享 → 24 个顶点。用「位置」去重会把它们错误合并，棱角被抹圆。',
        { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 4：UV 接缝 ============ */
  (function () {
    var g = root('fig-seam', 900, 330); if (!g) return;
    title(g, 30, 32, '循环曲面的接缝：多生成一圈，让 u 从 0.975 走到 1.0');

    /* --- 上：只生成 seg 列（错误）--- */
    var y1 = 92;
    txt(g, 30, y1 - 22, '✗  只生成 seg 列（0 .. seg−1）', { anchor: 'start', size: 12.5, weight: 700, color: C.accent2 });
    var xs1 = [70, 220, 370, 520, 640];
    var us1 = ['0.000', '0.250', '0.500', '0.750', '0.975'];
    for (var i = 0; i < xs1.length; i++) {
      circ(g, xs1[i], y1, 4.5, { fill: C.muted });
      txt(g, xs1[i], y1 - 14, us1[i], { size: 11, color: C.muted });
    }
    /* 最后一列与第一列重合：用往回的长虚线表示「倒流」 */
    var p1 = lerp([xs1[4], y1], [xs1[0], y1], 0); /* 不直接用，改用曲线示意 */
    g.appendChild(el('path', {
      d: 'M ' + xs1[4] + ' ' + (y1 + 10) + ' C ' + (xs1[4] - 60) + ' ' + (y1 + 62) +
         ', ' + (xs1[0] + 60) + ' ' + (y1 + 62) + ', ' + xs1[0] + ' ' + (y1 + 10),
      fill: 'none', stroke: C.accent2, 'stroke-width': 2, 'stroke-dasharray': '6 4'
    }));
    arrow(g, xs1[0] + 46, y1 + 55, xs1[0] + 6, y1 + 12, { color: C.accent2, w: 2, head: 7 });
    txt(g, (xs1[0] + xs1[4]) / 2, y1 + 46, '接缝三角形把整张贴图反向采样一遍 → 脏带',
        { size: 11.5, weight: 700, color: C.accent2 });
    txt(g, xs1[0] + 6, y1 + 86, '第 seg−1 列', { anchor: 'start', size: 10.5, color: C.muted });
    txt(g, xs1[4] - 6, y1 + 86, '第 0 列（与它几何相邻）', { anchor: 'end', size: 10.5, color: C.muted });

    /* --- 下：多生成一圈（正确）--- */
    var y2 = 246;
    txt(g, 30, y2 - 24, '✓  多生成一圈（0 .. seg）', { anchor: 'start', size: 12.5, weight: 700, color: GREEN });
    var xs2 = [70, 220, 370, 520, 640, 760];
    var us2 = ['0.000', '0.200', '0.400', '0.600', '0.800', '0.975', '1.000'];
    /* 用 7 个点太挤，取其中 6 个：0, 0.2, 0.4, 0.6, 0.8, 1.0，并在末尾补 0.975 说明 */
    var pts2 = [70, 190, 310, 430, 550, 700];
    var lab2 = ['0.000', '0.200', '0.400', '0.600', '0.800', '1.000'];
    for (var k = 0; k < pts2.length; k++) {
      circ(g, pts2[k], y2, 4.5, { fill: k === pts2.length - 1 ? GREEN : C.muted });
      txt(g, pts2[k], y2 - 14, lab2[k], {
        size: 11, color: k === pts2.length - 1 ? GREEN : C.muted,
        weight: k === pts2.length - 1 ? 700 : 400
      });
    }
    line(g, pts2[4], y2, pts2[5], y2, { color: GREEN, w: 3 });
    txt(g, 640, y2 + 26, '位置和第一列重合，但 u = 1.0', { anchor: 'start', size: 11.5, weight: 700, color: GREEN });
    txt(g, 70, y2 + 26, '接缝只剩 0.025 宽 → 正常的一列', { anchor: 'start', size: 11.5, color: C.muted });

    txt(g, 30, 316, '位置一样、UV 不同，就该是两个顶点 —— 这正是「顶点 = 属性的组合」的又一次体现。',
        { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 5：UV 翻转 ============ */
  (function () {
    var g = root('fig-uvflip', 660, 330); if (!g) return;
    title(g, 40, 32, 'OBJ 的 vt ⟷ 本项目 Texture 的 v 方向');

    /* 左：OBJ（原点左下，v 向上） */
    var L = { x: 60, y: 76, w: 190, h: 190 };
    rbox(g, L.x, L.y, L.w, L.h, { rx: 6, fill: 'rgba(217,116,26,.08)', stroke: C.accent2, w: 1.6 });
    txt(g, L.x + L.w / 2, L.y - 12, 'OBJ 的 vt 坐标系', { size: 12.5, weight: 700, color: C.accent2 });
    arrow(g, L.x - 14, L.y + L.h, L.x + L.w + 6, L.y + L.h, { color: C.muted, w: 1.4, head: 6 });
    arrow(g, L.x, L.y + L.h + 14, L.x, L.y - 6, { color: C.muted, w: 1.4, head: 6 });
    txt(g, L.x + L.w + 12, L.y + L.h + 4, 'u', { size: 11, color: C.muted });
    txt(g, L.x - 4, L.y - 4, 'v', { size: 11, color: C.muted });
    txt(g, L.x - 6, L.y + L.h + 16, '原点（左下）', { anchor: 'start', size: 10.5, color: C.muted });
    var py = L.y + L.h * (1 - 0.75);
    line(g, L.x, py, L.x + L.w, py, { color: C.accent2, w: 2, dash: '6 4' });
    circ(g, L.x + L.w * 0.25, py, 5, { fill: C.accent2 });
    txt(g, L.x + L.w + 8, py + 4, 'vt 0.25 0.75', { anchor: 'start', size: 11.5, weight: 700, color: C.accent2 });

    /* 中：翻转 */
    txt(g, 320, 168, '1 − v', { size: 17, weight: 700, color: C.accent2 });
    arrow(g, 278, 168, 300, 168, { color: C.muted, w: 2, head: 7 });
    arrow(g, 340, 168, 362, 168, { color: C.muted, w: 2, head: 7 });

    /* 右：本项目（原点左上，v 向下） */
    var R = { x: 388, y: 76, w: 190, h: 190 };
    rbox(g, R.x, R.y, R.w, R.h, { rx: 6, fill: 'rgba(47,91,234,.08)', stroke: C.accent, w: 1.6 });
    txt(g, R.x + R.w / 2, R.y - 12, '本项目 Texture 的 v 方向', { size: 12.5, weight: 700, color: C.accent });
    arrow(g, R.x - 14, R.y, R.x + R.w + 6, R.y, { color: C.muted, w: 1.4, head: 6 });
    arrow(g, R.x, R.y - 14, R.x, R.y + R.h + 6, { color: C.muted, w: 1.4, head: 6 });
    txt(g, R.x + R.w + 12, R.y + 4, 'u', { size: 11, color: C.muted });
    txt(g, R.x - 4, R.y + R.h + 12, 'v', { size: 11, color: C.muted });
    txt(g, R.x - 6, R.y - 18, '原点（左上）', { anchor: 'start', size: 10.5, color: C.muted });
    var py2 = R.y + R.h * 0.25;
    line(g, R.x, py2, R.x + R.w, py2, { color: C.accent, w: 2, dash: '6 4' });
    circ(g, R.x + R.w * 0.25, py2, 5, { fill: C.accent });
    txt(g, R.x + R.w + 8, py2 + 4, 'v = 0.25', { anchor: 'start', size: 11.5, weight: 700, color: C.accent });

    txt(g, 40, 300, '同一个点：OBJ 说「从下往上 75%」（靠顶部），本项目说「从上往下 25%」—— 落在图片同一个位置。',
        { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 40, 320, '贴图上下对称时（棋盘格）不翻也看不出来；木纹 / 人脸 / 带字的贴图立刻穿帮。',
        { anchor: 'start', size: 11.5, color: C.muted });
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
