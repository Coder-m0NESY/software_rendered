/* 第 11 步教程 · 插图库
   全部用内联 SVG 手绘，不依赖任何第三方库。
   这些是「几何 / 数据流原理图」，不是数据图表 —— 需要精确标注的坐标、
   平面位置和顶点顺序，所以直接用 SVG 画。
   设计系统与第 9、10 步教程保持一致（同一套 CSS 变量）。 */
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

  /* ============ 图 1：屏幕空间有向面积 ============ */
  (function () {
    var g = root('fig-winding', 620, 344); if (!g) return;
    title(g, 40, 34, '有向面积：凭顶点的绕序判正反');
    var panels = [
      { x0: 40, verdict: 'area2 > 0　→　正面，保留', color: C.accent,
        v: [[100, 118], [224, 118], [100, 248]] },
      { x0: 330, verdict: 'area2 < 0　→　背面，丢弃', color: C.accent2,
        v: [[390, 118], [390, 248], [514, 118]] }
    ];
    panels.forEach(function (p) {
      /* 屏幕坐标系：x 向右、y 向下 */
      arrow(g, p.x0, 80, p.x0 + 96, 80, { color: C.muted, w: 1.5, head: 7 });
      arrow(g, p.x0, 80, p.x0, 80 + 96, { color: C.muted, w: 1.5, head: 7 });
      txt(g, p.x0 + 104, 84, 'x', { anchor: 'start', size: 11.5, color: C.muted });
      txt(g, p.x0 + 5, 186, 'y', { anchor: 'start', size: 11.5, color: C.muted });

      poly(g, p.v, { fill: p.color === C.accent ? 'rgba(47,91,234,.10)' : 'rgba(217,116,26,.10)', stroke: p.color, w: 2.5 });

      /* 顶点 0/1/2 + 0→1→2 的顺序箭头 */
      for (var j = 0; j < 3; ++j) {
        var a = p.v[j], b = p.v[(j + 1) % 3];
        arrow(g, lerp(a, b, 0.22)[0], lerp(a, b, 0.22)[1],
                   lerp(a, b, 0.78)[0], lerp(a, b, 0.78)[1],
              { color: C.ink, w: 1.6, head: 7 });
      }
      for (var i = 0; i < 3; ++i) {
        circ(g, p.v[i][0], p.v[i][1], 6, { fill: p.color, stroke: C.bg, w: 2 });
        var dx = (p.v[i][0] === p.v[1][0] && p.v[i][1] === p.v[1][1]) ? 14
               : (p.v[i][0] === p.v[2][0] && p.v[i][1] === p.v[2][1]) ? -16 : 14;
        var dy = i === 0 ? -12 : 22;
        txt(g, p.v[i][0] + dx, p.v[i][1] + dy, String(i), { size: 12, weight: 700, color: C.ink });
      }
      txt(g, p.x0 + 120, 300, p.verdict, { size: 12, weight: 700, color: p.color });
    });
    txt(g, 310, 332, '同一个三角形，顶点顺序 0→1→2 一反过来，area2 的符号就翻转 —— 这就是判正反的全部依据。',
        { size: 11.5, color: C.muted });
  })();

  /* ============ 图 2：背面剔除在管线里的位置 ============ */
  (function () {
    var g = root('fig-pipeline', 900, 214); if (!g) return;
    title(g, 30, 36, '背面剔除在管线里的位置');
    var y = 66, h = 78, w = 118, gap = 22, x = 30;
    var steps = [
      ['顶点变换', '乘 MVP'],
      ['透视除法', '÷ w'],
      ['视口', '→ 屏幕坐标'],
      ['背面剔除', 'area2 ≤ 0 丢'],
      ['包围盒', '屏幕越界裁剪'],
      ['填像素', '写帧缓冲']
    ];
    for (var i = 0; i < steps.length; ++i) {
      var cx = x + i * (w + gap), hi = (i === 3);
      rbox(g, cx, y, w, h, { fill: hi ? 'rgba(217,116,26,.10)' : C.bg2, stroke: hi ? C.accent2 : C.rule, w: hi ? 2 : 1 });
      txt(g, cx + w / 2, y + 32, steps[i][0], { size: 13, weight: 700, color: hi ? C.accent2 : C.ink });
      txt(g, cx + w / 2, y + 54, steps[i][1], { size: 11, color: C.muted });
      if (i < steps.length - 1) arrow(g, cx + w + 3, y + h / 2, cx + w + gap - 3, y + h / 2, { color: C.muted, w: 1.8, head: 7 });
    }
    txt(g, 30, 178, '它必须发生在「视口」之后 —— 到这一步才有屏幕坐标，也才能用 area2 的符号判正反。',
        { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 30, 198, '而且它在「填像素」之前：整块丢掉，一个像素都不填，省的是填充率。',
        { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 3：近平面裁剪的侧视图 ============ */
  (function () {
    var g = root('fig-near', 640, 332); if (!g) return;
    title(g, 40, 34, '相机侧视图：三角形跨过近平面');
    var eyeX = 55, eyeY = 200, nearX = 190, farX = 580;

    /* 视锥楔形 */
    poly(g, [[eyeX, eyeY], [farX, 60], [farX, 300]], { fill: 'rgba(47,91,234,.06)', stroke: C.rule, w: 1.5 });

    /* 近平面 */
    line(g, nearX, 60, nearX, 302, { color: C.accent2, w: 2.5, dash: '7 5' });
    txt(g, nearX, 50, '近平面（z + w = 0）', { size: 12, weight: 700, color: C.accent2 });

    /* 相机 */
    circ(g, eyeX, eyeY, 6, { fill: C.ink });
    txt(g, eyeX, eyeY + 24, '相机', { size: 11.5, color: C.muted });

    /* 原三角形（虚线 = 被切掉的部分） */
    var vA = [112, 118], vB = [440, 92], vC = [410, 300];
    poly(g, [vA, vB, vC], { fill: 'none', stroke: C.rule, w: 1.5, dash: '6 4' });

    var P1 = lerp(vA, vB, (nearX - vA[0]) / (vB[0] - vA[0]));
    var P2 = lerp(vA, vC, (nearX - vA[0]) / (vC[0] - vA[0]));

    /* 保留部分 */
    poly(g, [P1, vB, vC, P2], { fill: 'rgba(57,162,75,.16)', stroke: GREEN, w: 2.5 });

    circ(g, vA[0], vA[1], 5, { fill: C.accent2 });
    txt(g, vA[0] - 8, vA[1] + 20, '顶点在平面背后 → 切掉', { anchor: 'middle', size: 11, color: C.accent2 });
    circ(g, P1[0], P1[1], 5, { fill: GREEN, stroke: C.bg, w: 2 });
    circ(g, P2[0], P2[1], 5, { fill: GREEN, stroke: C.bg, w: 2 });
    txt(g, 500, 210, '保留', { anchor: 'start', size: 12, weight: 700, color: GREEN });
    txt(g, 40, 316, '近平面左边（相机与平面之间）那块要切掉；右边那块保留。切完的碎片再做透视除法，w 一定 > 0，除法安全。',
        { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 4：Sutherland-Hodgman 的产物 ============ */
  (function () {
    var g = root('fig-sh', 640, 336); if (!g) return;
    title(g, 40, 34, '走一圈边 → 得到 4 个点 → 扇成 2 个三角形');
    var nearX = 195, vA = [120, 130], vB = [455, 96], vC = [420, 300];

    line(g, nearX, 66, nearX, 268, { color: C.accent2, w: 2.5, dash: '7 5' });
    txt(g, nearX, 54, '近平面', { size: 12, weight: 700, color: C.accent2 });

    var P1 = lerp(vA, vB, (nearX - vA[0]) / (vB[0] - vA[0]));
    var P2 = lerp(vA, vC, (nearX - vA[0]) / (vC[0] - vA[0]));

    /* 扇出的两个三角形 */
    poly(g, [P1, vB, vC], { fill: 'rgba(47,91,234,.13)', stroke: C.accent, w: 2 });
    poly(g, [P1, vC, P2], { fill: 'rgba(57,162,75,.16)', stroke: GREEN, w: 2 });
    line(g, P1[0], P1[1], vC[0], vC[1], { color: C.muted, w: 1.4, dash: '5 4' });

    /* 四个输出点，按算法产生顺序标号 */
    var out = [[P1, 'poly[0]（交点）', C.accent2], [vB, 'poly[1] = B', C.ink],
               [vC, 'poly[2] = C', C.ink], [P2, 'poly[3]（交点）', C.accent2]];
    out.forEach(function (o, i) {
      circ(g, o[0][0], o[0][1], 5.5, { fill: o[2], stroke: C.bg, w: 2 });
      var lx = o[0][0] + (i < 2 ? 14 : 14), ly = o[0][1] + (i === 1 ? -12 : (i === 0 ? 22 : 24));
      txt(g, lx, ly, o[1], { anchor: 'start', size: 11, color: o[2] });
    });

    txt(g, 40, 292, '三角形 1：poly[0] + poly[1] + poly[2]', { anchor: 'start', size: 11.5, weight: 700, color: C.accent });
    txt(g, 40, 310, '三角形 2：poly[0] + poly[2] + poly[3]（扇心固定是 poly[0]）', { anchor: 'start', size: 11.5, weight: 700, color: GREEN });
    txt(g, 40, 328, '切分不改变绕序 —— 切出来的碎块照样能做背面剔除。', { anchor: 'start', size: 11, color: C.muted });
  })();

  /* ============ 图 5：裁剪 → 透视除法 的顺序 ============ */
  (function () {
    var g = root('fig-order', 900, 214); if (!g) return;
    title(g, 30, 36, '为什么裁剪必须在透视除法之前');
    var y = 62, h = 88, w = 146, gap = 30, x = 30;
    var boxes = [
      ['乘 MVP', ['裁剪空间 (x,y,z,w)', '线性、无除法'], C.accent],
      ['近平面裁剪', ['在这里切掉背后那块', '插值天生透视正确'], C.accent2],
      ['透视除法', ['÷ w → NDC', 'w ≤ 0 会翻号、会炸'], C.rule],
      ['视口', ['→ 屏幕坐标', ''], C.rule],
      ['光栅化', ['填三角形', ''], C.rule]
    ];
    for (var i = 0; i < boxes.length; ++i) {
      var cx = x + i * (w + gap), hi = (i === 1);
      rbox(g, cx, y, w, h, { fill: hi ? 'rgba(217,116,26,.10)' : C.bg2, stroke: hi ? C.accent2 : C.rule, w: hi ? 2 : 1 });
      g.appendChild(el('rect', { x: cx, y: y, width: 4, height: h, rx: 2, fill: boxes[i][2], stroke: 'none' }));
      txt(g, cx + w / 2 + 2, y + 30, boxes[i][0], { size: 13, weight: 700, color: hi ? C.accent2 : C.ink });
      txt(g, cx + w / 2 + 2, y + 52, boxes[i][1][0], { size: 11, color: C.muted });
      if (boxes[i][1][1]) txt(g, cx + w / 2 + 2, y + 70, boxes[i][1][1], { size: 11, color: C.muted });
      if (i < boxes.length - 1) arrow(g, cx + w + 4, y + h / 2, cx + w + gap - 4, y + h / 2, { color: C.muted, w: 2, head: 7 });
    }
    txt(g, 30, 184, '顺序反了会怎样：先在裁剪空间丢掉 w ≤ 0 的点，虽然也能编译，但你会失去「把三角形切开保留一半」的能力 —— 地板就会缺块。',
        { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 30, 204, '正确做法：先切、再除。切完的顶点 w ≥ near > 0，除法永远安全。', { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 6：视锥体与 6 个平面 ============ */
  (function () {
    var g = root('fig-frustum', 620, 330); if (!g) return;
    title(g, 40, 34, '俯视图：视锥体 = 相机前方的一块空间，被 6 个平面围住');
    var eyeX = 70, eyeY = 190, nearX = 175, farX = 560;

    /* 视锥（俯视，梯形） */
    poly(g, [[nearX, 138], [farX, 70], [farX, 310], [nearX, 242]],
         { fill: 'rgba(47,91,234,.08)', stroke: C.accent, w: 2 });

    /* 相机到近平面 */
    poly(g, [[eyeX, eyeY], [nearX, 138], [nearX, 242]], { fill: 'none', stroke: C.rule, w: 1.2, dash: '5 4' });
    circ(g, eyeX, eyeY, 6, { fill: C.ink });
    txt(g, eyeX, eyeY + 24, '相机', { size: 11.5, color: C.muted });

    /* 近 / 远 平面 */
    line(g, nearX, 138, nearX, 242, { color: C.accent2, w: 3 });
    txt(g, nearX, 128, '近', { size: 12, weight: 700, color: C.accent2 });
    line(g, farX, 70, farX, 310, { color: '#b455c9', w: 3 });
    txt(g, farX, 60, '远', { size: 12, weight: 700, color: '#b455c9' });

    /* 左 / 右 平面标注 */
    txt(g, 300, 96, '左平面', { size: 12, weight: 700, color: C.accent });
    txt(g, 300, 296, '右平面', { size: 12, weight: 700, color: C.accent });

    /* 盒子 */
    g.appendChild(el('rect', { x: 250, y: 150, width: 60, height: 60, fill: 'rgba(57,162,75,.18)', stroke: GREEN, 'stroke-width': 2 }));
    txt(g, 280, 228, '物体包围盒', { size: 11.5, weight: 700, color: GREEN });

    var g2 = g;
    txt(g2, 40, 316, '还有「下」「上」两个平面垂直于纸面（图中看不到）。只要盒子完全落在任意一个平面的外侧，就整个跳过。',
        { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 7：p-vertex 测试 ============ */
  (function () {
    var g = root('fig-pvertex', 620, 330); if (!g) return;
    title(g, 40, 34, 'p-vertex 测试：取「法线方向最远的角」');
    var planeX = 250;
    /* 平面（斜的） */
    line(g, planeX - 30, 320, planeX + 40, 40, { color: C.accent2, w: 2.5 });
    txt(g, planeX - 26, 300, '平面', { anchor: 'end', size: 11.5, weight: 700, color: C.accent2 });
    /* 法线（指向内侧 = 右侧） */
    arrow(g, planeX, 190, planeX + 62, 190, { color: C.accent, w: 2.2, head: 8 });
    txt(g, planeX + 70, 194, '法线（指向内侧）', { anchor: 'start', size: 11.5, color: C.accent });

    /* 盒子 */
    var bx = 280, by = 110, bw = 210, bh = 150;
    g.appendChild(el('rect', { x: bx, y: by, width: bw, height: bh, fill: 'rgba(47,91,234,.08)', stroke: C.ink, 'stroke-width': 2 }));

    /* p-vertex：沿法线最远 = 右下角；n-vertex：左上角 */
    var pv = [bx + bw, by + bh], nv = [bx, by];
    circ(g, pv[0], pv[1], 6, { fill: C.accent, stroke: C.bg, w: 2 });
    txt(g, pv[0] - 6, pv[1] + 24, 'p-vertex（最远）', { anchor: 'end', size: 11.5, weight: 700, color: C.accent });
    circ(g, nv[0], nv[1], 6, { fill: C.accent2, stroke: C.bg, w: 2 });
    txt(g, nv[0] - 6, nv[1] - 10, 'n-vertex（最近）', { anchor: 'end', size: 11.5, weight: 700, color: C.accent2 });

    txt(g, 40, 300, '只测 p-vertex：连「最靠里」的那个角都在平面外 → 整个盒子一定在外 → 剔除。', { anchor: 'start', size: 11.5, color: C.ink });
    txt(g, 40, 320, '绝不能取 n-vertex：它更靠外，会有一部分还露在视锥里的盒子被误判成完全在外 —— 漏画。', { anchor: 'start', size: 11.5, color: C.accent2 });
  })();

  /* ============ 图 8：绕序 bug 的几何 ============ */
  (function () {
    var g = root('fig-bug', 620, 330); if (!g) return;
    title(g, 40, 34, '左 / 右面：顶点缠绕方向和其余四面相反');
    /* 等轴测立方体 */
    var F = [[150, 120], [310, 120], [310, 280], [150, 280]];   // 前面
    var ox = 92, oy = -62;                                      // 后面的偏移
    var Bk = F.map(function (p) { return [p[0] + ox, p[1] + oy]; });

    /* 六个面里，前面 + 顶面 + 后面画常规色 */
    poly(g, [F[0], F[1], F[2], F[3]], { fill: 'rgba(47,91,234,.10)', stroke: C.ink, w: 2 });
    poly(g, [F[0], Bk[0], Bk[1], F[1]], { fill: 'rgba(47,91,234,.06)', stroke: C.ink, w: 2 });
    poly(g, [Bk[0], Bk[1], Bk[2], Bk[3]], { fill: 'rgba(47,91,234,.04)', stroke: C.ink, w: 1.5, dash: '5 4' });
    line(g, F[3][0], F[3][1], Bk[3][0], Bk[3][1], { color: C.ink, w: 2 });
    line(g, F[2][0], F[2][1], Bk[2][0], Bk[2][1], { color: C.ink, w: 2 });

    /* 左面（-X）与右面（+X）高亮 */
    var left = [F[0], Bk[0], Bk[3], F[3]];     // 左面
    var right = [F[1], Bk[1], Bk[2], F[2]];    // 右面
    poly(g, left, { fill: 'rgba(217,116,26,.22)', stroke: C.accent2, w: 2.5 });
    poly(g, right, { fill: 'rgba(217,116,26,.22)', stroke: C.accent2, w: 2.5 });

    txt(g, 470, 150, '这两面', { anchor: 'start', size: 12.5, weight: 700, color: C.accent2 });
    txt(g, 470, 172, '缠绕方向反了', { anchor: 'start', size: 12.5, weight: 700, color: C.accent2 });
    txt(g, 470, 200, '→ 背面剔除会把它们', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 470, 218, '　 当成正面留下来', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 470, 244, '→ 贴脸时正面被近平面', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 470, 262, '　 切掉，它们就露出来了', { anchor: 'start', size: 11.5, color: C.muted });

    txt(g, 40, 316, '前 10 步没有背面剔除，这两个错误的侧面被深度测试挡在后面，一直没被发现。', { anchor: 'start', size: 11.5, color: C.muted });
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
