/* 第 13 步教程 · 插图库
   全部用内联 SVG 手绘，不依赖任何第三方库。
   这些是「原理 / 数据流示意图」，不是数据图表 —— 需要精确的颜色值、
   框线与下标对应关系，所以直接用 SVG 画。
   设计系统与第 9 ~ 12 步教程保持一致（同一套 CSS 变量）。 */
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
  var RED = '#c0392b';
  var BLUE = '#2f5bea';

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
      stroke: o.stroke || 'none', 'stroke-width': o.w || 1.5,
      'stroke-dasharray': o.dash || 'none'
    }));
  }
  function rbox(g, x, y, w, h, o) {
    o = o || {};
    g.appendChild(el('rect', {
      x: x, y: y, width: w, height: h, rx: o.rx == null ? 10 : o.rx,
      fill: o.fill || 'none', stroke: o.stroke || C.rule,
      'stroke-width': o.w || 1, 'stroke-dasharray': o.dash || 'none'
    }));
  }
  function title(g, x, y, s, o) {
    txt(g, x, y, s, { anchor: 'start', size: 12.5, weight: 600, color: (o && o.color) || C.accent2 });
  }
  function rgb(c) { return 'rgb(' + c.join(',') + ')'; }
  function mul(c, a) { return [Math.round(c[0] * a), Math.round(c[1] * a), Math.round(c[2] * a)]; }
  /* over：源 × α + 目标 × (1−α)，中间不取整，最后一步才取整（与代码一致） */
  function over(src, dst, a) {
    return [
      Math.round(src[0] * a + dst[0] * (1 - a)),
      Math.round(src[1] * a + dst[1] * (1 - a)),
      Math.round(src[2] * a + dst[2] * (1 - a))
    ];
  }

  /* ============ 图 1：Alpha 测试「一刀切」 vs 颜色混合「调浓度」============ */
  (function () {
    var g = root('fig-two-effects', 900, 344); if (!g) return;
    title(g, 30, 30, '同一个「带 alpha 的贴图」：左边只有 0 / 1，右边是连续值');

    /* --- 左：Alpha 测试 --- */
    txt(g, 40, 62, 'Alpha 测试 · 一刀切', { anchor: 'start', size: 13, weight: 700, color: C.accent2 });
    var alphas = [30, 70, 110, 150, 190, 230, 255, 90];
    var bx = 40, by = 82, bw = 44, bh = 92, gap = 6;
    var thr = 128;
    alphas.forEach(function (a, i) {
      var x = bx + i * (bw + gap);
      var keep = a >= thr;
      if (keep) {
        rbox(g, x, by, bw, bh, { rx: 6, fill: rgb(mul([230, 150, 60], 1)), stroke: C.rule });
        txt(g, x + bw / 2, by + bh / 2 + 5, '留', { size: 15, weight: 700, color: '#fff' });
      } else {
        rbox(g, x, by, bw, bh, { rx: 6, fill: 'none', stroke: C.accent2, w: 1.6, dash: '5 4' });
        txt(g, x + bw / 2, by + bh / 2 - 4, '丢', { size: 15, weight: 700, color: C.accent2 });
        txt(g, x + bw / 2, by + bh / 2 + 16, '不写深度', { size: 9.5, color: C.muted });
      }
      txt(g, x + bw / 2, by + bh + 18, String(a), {
        size: 10.5, color: keep ? C.ink : C.muted, weight: keep ? 700 : 400
      });
    });
    line(g, bx, by - 6, bx + alphas.length * (bw + gap) - gap, by - 6, { color: C.muted, w: 1, dash: '4 4' });
    txt(g, bx + 8, by - 12, '阈值 128', { anchor: 'start', size: 10.5, color: C.muted });
    txt(g, 40, by + bh + 44, '每个片元只有两种命运：整个留下，或整个丢掉。', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 40, by + bh + 62, '丢掉 = 不写颜色、不写深度 —— 洞是真的洞。', { anchor: 'start', size: 11.5, color: C.muted });

    /* 中缝 */
    line(g, 448, 52, 448, 300, { color: C.rule, w: 1.5, dash: '6 6' });

    /* --- 右：颜色混合 --- */
    var rx = 470;
    txt(g, rx, 62, '颜色混合 · 调浓度', { anchor: 'start', size: 13, weight: 700, color: C.accent });
    /* alpha 渐变条：左 = 目标色，右 = 源色 */
    var dst = [40, 70, 120], src = [230, 150, 60];
    var defs = el('defs', {});
    var lg = el('linearGradient', { id: 'g-blend', x1: '0', y1: '0', x2: '1', y2: '0' });
    lg.appendChild(el('stop', { offset: '0', 'stop-color': rgb(dst) }));
    lg.appendChild(el('stop', { offset: '1', 'stop-color': rgb(src) }));
    defs.appendChild(lg);
    g.appendChild(defs);
    g.appendChild(el('rect', {
      x: rx, y: by, width: 400, height: bh, rx: 6,
      fill: 'url(#g-blend)', stroke: C.rule
    }));
    txt(g, rx + 8, by + bh - 10, 'α=0', { anchor: 'start', size: 10.5, color: '#fff', weight: 700 });
    txt(g, rx + 392, by + bh - 10, 'α=1', { anchor: 'end', size: 10.5, color: '#fff', weight: 700 });
    txt(g, rx + 200, by + bh / 2 + 5, '源 × α + 目标 × (1−α)', { size: 13, weight: 700, color: '#fff' });
    txt(g, rx, by + bh + 18, '0 ~ 255 连续', { anchor: 'start', size: 10.5, color: C.muted });
    txt(g, rx, by + bh + 44, '片元一定留下，但不是覆盖：跟已经画好的底色加权平均。', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, rx, by + bh + 62, '这就是玻璃、水、火焰「能看见背后」的原因。', { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 2：混合公式 —— α 从 0.9 到 0.1 ============ */
  (function () {
    var g = root('fig-blend-math', 760, 364); if (!g) return;
    title(g, 30, 30, '把源色按 α = 0.9 / 0.5 / 0.3 / 0.1 铺到「黑底」和「红底」上');

    var src = [230, 150, 60];
    var black = [0, 0, 0];
    var red = [190, 50, 60];
    var A = [0.9, 0.5, 0.3, 0.1];

    /* 图例 */
    rbox(g, 30, 48, 22, 22, { rx: 4, fill: rgb(src), stroke: C.rule });
    txt(g, 60, 64, '源（source）', { anchor: 'start', size: 11.5, color: C.muted });
    rbox(g, 190, 48, 22, 22, { rx: 4, fill: rgb(black), stroke: C.rule });
    txt(g, 220, 64, '黑底', { anchor: 'start', size: 11.5, color: C.muted });
    rbox(g, 300, 48, 22, 22, { rx: 4, fill: rgb(red), stroke: C.rule });
    txt(g, 330, 64, '红底', { anchor: 'start', size: 11.5, color: C.muted });

    var barW = 118, gap = 26, x0 = 196;
    function row(label, dst, y, labColor) {
      txt(g, 30, y + 40, label, { anchor: 'start', size: 12.5, weight: 700, color: labColor });
      rbox(g, 30, y + 52, 60, 40, { rx: 6, fill: rgb(dst), stroke: C.rule });
      A.forEach(function (a, i) {
        var x = x0 + i * (barW + gap);
        var out = over(src, dst, a);
        rbox(g, x, y, barW, 92, { rx: 6, fill: rgb(out), stroke: C.rule });
        txt(g, x + barW / 2, y + 34, 'α=' + a.toFixed(1), { size: 12, weight: 700, color: '#fff' });
        txt(g, x + barW / 2, y + 66, 'rgb(' + out.join(',') + ')', { size: 10, color: '#fff' });
      });
    }
    row('黑底上', black, 90, C.ink);
    row('红底上', red, 216, C.ink);

    txt(g, 30, 344, 'α 越接近 1 越像源（覆盖），越接近 0 越像底（透掉）。', { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 3：混合不可交换 ============ */
  (function () {
    var g = root('fig-not-commutative', 900, 372); if (!g) return;
    title(g, 30, 30, '同一对半透明色，换个盖的顺序 → 最终颜色不同');

    var cA = [230, 60, 60], aA = 0.9;   /* 红 α=0.9 */
    var cB = [60, 90, 230], aB = 0.3;   /* 蓝 α=0.3 */
    var C0 = [245, 245, 248];           /* 白底 */

    /* 底色 */
    rbox(g, 30, 96, 76, 76, { rx: 8, fill: rgb(C0), stroke: C.rule });
    txt(g, 68, 142, 'C₀', { size: 14, weight: 700, color: C.ink });
    txt(g, 68, 188, '白底', { size: 11, color: C.muted });

    function swatch(x, y, col, a, name) {
      rbox(g, x, y, 76, 76, { rx: 8, fill: rgb(col), stroke: C.rule });
      /* 用底纹表示透明度 */
      g.appendChild(el('rect', {
        x: x, y: y, width: 76, height: 76, rx: 8,
        fill: rgb(col), opacity: a.toFixed(2), stroke: C.rule
      }));
      txt(g, x + 38, y - 10, name, { size: 11.5, weight: 700, color: C.ink });
      txt(g, x + 38, y + 92, 'α=' + a, { size: 10.5, color: C.muted });
    }

    /* 行一：先 A 后 B */
    var y1 = 96, y2 = 236;
    var r1 = over(cB, over(cA, C0, aA), aB);
    var r2 = over(cA, over(cB, C0, aB), aA);

    txt(g, 130, y1 - 26, '先盖 A（红）→ 再盖 B（蓝）', { anchor: 'start', size: 12.5, weight: 700, color: C.accent });
    swatch(150, y1, cA, aA, 'A 红');
    arrow(g, 232, y1 + 38, 258, y1 + 38, { color: C.muted, w: 2, head: 7 });
    swatch(264, y1, cB, aB, 'B 蓝');
    arrow(g, 346, y1 + 38, 372, y1 + 38, { color: C.muted, w: 2, head: 7 });
    rbox(g, 378, y1, 96, 76, { rx: 8, fill: rgb(r1), stroke: C.ink, w: 2 });
    txt(g, 426, y1 - 10, '结果', { size: 11.5, weight: 700, color: C.ink });
    txt(g, 426, y1 + 92, 'rgb(' + r1.join(',') + ')', { size: 10.5, color: C.muted });

    /* 行二：先 B 后 A */
    txt(g, 130, y2 - 26, '先盖 B（蓝）→ 再盖 A（红）', { anchor: 'start', size: 12.5, weight: 700, color: C.accent2 });
    swatch(150, y2, cB, aB, 'B 蓝');
    arrow(g, 232, y2 + 38, 258, y2 + 38, { color: C.muted, w: 2, head: 7 });
    swatch(264, y2, cA, aA, 'A 红');
    arrow(g, 346, y2 + 38, 372, y2 + 38, { color: C.muted, w: 2, head: 7 });
    rbox(g, 378, y2, 96, 76, { rx: 8, fill: rgb(r2), stroke: C.ink, w: 2 });
    txt(g, 426, y2 - 10, '结果', { size: 11.5, weight: 700, color: C.ink });
    txt(g, 426, y2 + 92, 'rgb(' + r2.join(',') + ')', { size: 10.5, color: C.muted });

    /* 结论 */
    rbox(g, 540, y1, 330, y2 + 76 - y1, { rx: 10, fill: C.bg2, stroke: C.rule });
    txt(g, 560, y1 + 34, 'A 混 B ≠ B 混 A', { anchor: 'start', size: 15, weight: 700, color: C.accent });
    txt(g, 560, y1 + 66, 'over 不满足交换律。', { anchor: 'start', size: 12.5, color: C.ink });
    txt(g, 560, y1 + 92, '每一步都在「往当前背景上再盖一层」。', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 560, y1 + 112, '后盖的纸，盖在「已经含前一张纸」的底上。', { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 560, y1 + 146, '于是：顺序 = 从最远到最近。', { anchor: 'start', size: 12.5, weight: 700, color: C.accent2 });
  })();

  /* ============ 图 4：绘制顺序 ============ */
  (function () {
    var g = root('fig-order', 900, 384); if (!g) return;
    title(g, 30, 30, '一帧的绘制顺序：不透明整体在前，半透明整体在后、由远及近');

    /* 相机 */
    circ(g, 70, 150, 20, { fill: C.bg2, stroke: C.accent, w: 2 });
    circ(g, 70, 150, 7, { fill: C.accent });
    txt(g, 70, 190, '眼睛', { size: 11, color: C.muted });
    arrow(g, 96, 150, 850, 150, { color: C.muted, w: 1.4, dash: '6 5', head: 6 });
    txt(g, 500, 138, '视线方向', { size: 10.5, color: C.muted });

    /* 不透明物体（实心方块） */
    function cube(x, s, col) {
      rbox(g, x, 150 - s / 2, s, s, { rx: 5, fill: rgb(col), stroke: C.rule });
    }
    cube(180, 46, [70, 90, 140]);
    cube(268, 46, [90, 110, 150]);
    cube(356, 46, [110, 120, 150]);
    txt(g, 268, 118, '不透明物体', { size: 11.5, weight: 700, color: C.accent });

    /* 半透明物体（空心圆） */
    function ball(x, r, col) {
      circ(g, x, 150, r, { fill: rgb(col), stroke: C.rule });
      circ(g, x, 150, r, { fill: rgb(col), opacity: 0.45 });
    }
    ball(520, 24, [60, 170, 190]);
    ball(652, 28, [230, 160, 60]);
    ball(784, 32, [190, 90, 140]);
    txt(g, 652, 108, '半透明物体', { size: 11.5, weight: 700, color: C.accent2 });
    /* 距离标注 */
    var bx = [520, 652, 784], bl = ['近', '中', '远'];
    bx.forEach(function (x, i) {
      txt(g, x, 216, bl[i], { size: 10.5, color: C.muted });
      txt(g, x, 232, '(' + (i === 0 ? '最近' : i === 1 ? '中间' : '最远') + ')', { size: 10, color: C.muted });
    });

    /* 下方：绘制顺序条 */
    var y = 268;
    txt(g, 30, y - 8, '实际绘制顺序（左 → 右）', { anchor: 'start', size: 12.5, weight: 700, color: C.ink });
    var segW = [260, 174, 174, 174];
    var seg = [
      { t: '① 不透明（3 个）', d: '彼此顺序无所谓（深度测试兜底）', col: C.accent, bg: 'rgba(47,91,234,.10)' },
      { t: '② 半透明 · 最远',  d: '先画最远的', col: C.accent2, bg: 'rgba(217,116,26,.10)' },
      { t: '③ 半透明 · 中间',  d: '再画中间的', col: C.accent2, bg: 'rgba(217,116,26,.10)' },
      { t: '④ 半透明 · 最近',  d: '最后画最近的', col: C.accent2, bg: 'rgba(217,116,26,.10)' }
    ];
    var x = 60;
    seg.forEach(function (s) {
      rbox(g, x, y, segW[seg.indexOf(s)], 54, { rx: 8, fill: s.bg, stroke: s.col, w: 1.6 });
      txt(g, x + segW[seg.indexOf(s)] / 2, y + 24, s.t, { size: 12, weight: 700, color: s.col });
      txt(g, x + segW[seg.indexOf(s)] / 2, y + 42, s.d, { size: 10.5, color: C.muted });
      x += segW[seg.indexOf(s)] + 6;
    });

    txt(g, 30, 360, '排序 = build_draw_order(eye)：先按 transparent 分成两拨，再把半透明那拨按到相机的平方距离降序排。',
        { anchor: 'start', size: 11.5, color: C.muted });
  })();

  /* ============ 图 5：片元处理六步 ============ */
  (function () {
    var g = root('fig-frag-steps', 900, 300); if (!g) return;
    title(g, 30, 30, '一个片元的处理顺序：先判断「留不留」，再决定「怎么留」，最后「登记归谁」');

    var steps = [
      { n: '①', name: '插值 z',      rw: '读 uv / z',        col: C.muted,   note: '' },
      { n: '②', name: '深度测试',    rw: '读 depth',         col: C.accent,  note: '更远就丢' },
      { n: '③', name: '采样 + 光照', rw: '读贴图 → src',     col: GREEN,     note: '拿到颜色和 alpha' },
      { n: '④', name: 'Alpha 测试',  rw: '读 src.a',         col: C.accent2, note: '不达标整个丢' },
      { n: '⑤', name: '颜色混合',    rw: '读+写 color',      col: RED,       note: '与底色加权平均' },
      { n: '⑥', name: '写深度',      rw: '写 depth',         col: '#7a5bd0', note: '半透明传 false' }
    ];
    var bw = 136, gap = 12, x0 = 22, y = 92, bh = 118;
    steps.forEach(function (s, i) {
      var x = x0 + i * (bw + gap);
      rbox(g, x, y, bw, bh, { rx: 8, fill: C.bg2, stroke: s.col, w: 1.8 });
      circ(g, x + bw / 2, y + 26, 14, { fill: s.col });
      txt(g, x + bw / 2, y + 31, s.n, { size: 14, weight: 700, color: '#fff' });
      txt(g, x + bw / 2, y + 62, s.name, { size: 12.5, weight: 700, color: C.ink });
      txt(g, x + bw / 2, y + 84, s.rw, { size: 10.5, color: s.col, weight: 600 });
      if (s.note) txt(g, x + bw / 2, y + 104, s.note, { size: 10, color: C.muted });
      if (i < steps.length - 1) {
        arrow(g, x + bw + 1, y + bh / 2, x + bw + gap - 1, y + bh / 2, { color: C.rule, w: 2, head: 6 });
      }
    });

    txt(g, 22, 250, '关键点：④ Alpha 测试必须在 ③ 之后（要读 src.a），⑤ 混合必须在 ⑥ 写深度之前（半透明靠「不写深度」让后面的层叠上来）。',
        { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 22, 272, '如果把 ④ 放到 ⑤ 后面：本该被丢掉的片元已经跟背景混了一半，continue 追不回来 → 脏颜色。',
        { anchor: 'start', size: 11.5, color: C.accent2, weight: 600 });
  })();

  /* ============ 图 6：DrawOptions = 状态对象（PSO）============ */
  (function () {
    var g = root('fig-state', 880, 392); if (!g) return;
    title(g, 30, 30, '把「画什么（数据）」和「怎么画（状态）」分开 —— 现代图形 API 的核心思想');

    /* 左：数据 / 资源 */
    var lx = 30, ly = 70, lw = 250;
    rbox(g, lx, ly, lw, 250, { rx: 10, fill: 'rgba(47,91,234,.06)', stroke: C.accent, w: 1.6 });
    txt(g, lx + lw / 2, ly + 28, '画什么 · 数据 / 资源', { size: 13.5, weight: 700, color: C.accent });
    var data = [
      'Mesh：顶点数组 + 索引数组',
      'Texture：贴图 + alpha 通道',
      'Mat44：model / view / proj',
      '光照：光源方向 + 颜色'
    ];
    data.forEach(function (s, i) {
      var yy = ly + 58 + i * 44;
      rbox(g, lx + 18, yy, lw - 36, 34, { rx: 6, fill: C.bg, stroke: C.rule });
      txt(g, lx + lw / 2, yy + 22, s, { size: 11.5, color: C.ink });
    });

    /* 右：状态 / DrawOptions */
    var rx = 330, rw = 250;
    rbox(g, rx, ly, rw, 250, { rx: 10, fill: 'rgba(217,116,26,.07)', stroke: C.accent2, w: 1.6 });
    txt(g, rx + rw / 2, ly + 28, '怎么画 · 状态 DrawOptions', { size: 13.5, weight: 700, color: C.accent2 });
    var opts = [
      'perspective_correct',
      'bilinear',
      'alpha_test / threshold',
      'blend',
      'write_depth'
    ];
    opts.forEach(function (s, i) {
      var yy = ly + 58 + i * 38;
      rbox(g, rx + 18, yy, rw - 36, 30, { rx: 6, fill: C.bg, stroke: C.rule });
      txt(g, rx + rw / 2, yy + 20, s, { size: 11, color: C.ink });
    });

    /* 中间箭头：绑定 */
    arrow(g, lx + lw + 4, ly + 125, rx - 4, ly + 125, { color: C.ink, w: 2.4, head: 9 });
    txt(g, (lx + lw + rx) / 2, ly + 112, '绑定', { size: 11, color: C.muted });

    /* 结果 */
    var ox = 620, ow = 230;
    rbox(g, ox, ly + 70, ow, 110, { rx: 10, fill: C.bg2, stroke: C.ink, w: 2 });
    txt(g, ox + ow / 2, ly + 104, '一次 draw call', { size: 14, weight: 700, color: C.ink });
    txt(g, ox + ow / 2, ly + 130, '数据 × 状态 → 输出', { size: 11.5, color: C.muted });
    txt(g, ox + ow / 2, ly + 154, '换状态 = 换一套画法', { size: 11.5, color: C.accent2 });
    arrow(g, rx + rw + 4, ly + 125, ox - 4, ly + 125, { color: C.ink, w: 2.4, head: 9 });

    /* 对照 */
    txt(g, 30, 356, '↔ D3D12 PSO（Pipeline State Object）· Vulkan VkPipeline · OpenGL 一组 glEnable / glBlendFunc',
        { anchor: 'start', size: 11.5, color: C.muted });
    txt(g, 30, 378, '为什么打包：两个 bool 挨着写，调用点传反了编译器一句话都不说；打包成对象后，字段名自带语义。',
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
