// SÉANCE - an Arcade Phantasm game.
// A side-view platformer where you summon your past. Run the level, then rewind: time goes back to the start of the
// attempt and the run you just made rises as a ghost that repeats it frame by frame. Your ghosts hold plates, ride
// lifts and lend you their heads to stand on. The physics runs at a fixed 60 steps a second, so every replay is exact.
(function () {
  "use strict";
  var W = 960, H = 540, T = 30, COLS = 32, ROWS = 18;
  var PW = 18, PH = 26;                                   // the living and the dead are the same size
  var GRAV = 0.45, JUMP = -9, RUN = 3.2, ACC_G = 0.6, ACC_A = 0.4, MAXFALL = 10, CUT = -3.5, COYOTE = 6, BUFFER = 6;
  var IN_L = 1, IN_R = 2, IN_J = 4;
  var STEP = 1000 / 60;
  var C = { navy: "#001725", deep: "#005184", blue: "#007ab7", sky: "#34a9e3", pink: "#f088e9", green: "#39ff6a",
            black: "#020611", ghost: "#cfeeff", white: "#ffffff" };
  var ROMAN = ["I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"];

  // ---- the levels -------------------------------------------------------------------------------------------------
  // # wall   - ledge you can jump up through   ^ spikes   P start   E the way out   i a candle
  // a-e pressure plates   A-E doors (open while every plate / lever with the same letter is held)   ! a lever
  // (a lever keeps its door open for a while after you leave it). Lifts are listed under the map: they rise to y2
  // while their letter is held and sink back when it isn't.
  var FULL = "################################";
  var LEVELS = [
    { name: "First Light", ghosts: 1,
      hint: "Stand on the plate and press R. Your past self will hold the door while you walk through.",
      map: [FULL, FULL, FULL, FULL,
        "######....................######",
        "####........................####",
        "###..........................###",
        "##............................##",
        "##............................##",
        "#..............................#",
        "#......................#########",
        "#......................#.......#",
        "#...i..........i.......#...i...#",
        "#......................A.......#",
        "#......................A.......#",
        "#..P..a......^^........A....E..#",
        FULL, FULL] },
    { name: "Ascension", ghosts: 1,
      hint: "A lift rises while its plate is held. Leave a ghost on the plate, then ride.",
      lifts: [{ c: 4, w: 2, y: 480, y2: 120, link: "b" }],
      map: [FULL,
        "#..............................#",
        "#..............................#",
        "#.E.i..........................#",
        "####...........................#",
        "####...........................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#.....i...........i............#",
        "#..............................#",
        "#..............................#",
        "#......P..............^^^...b..#",
        "####..##########################",
        FULL] },
    { name: "Stand on Me", ghosts: 1,
      hint: "The living can stand on the dead. Leave a ghost by the wall and climb.",
      map: [FULL, FULL, FULL, FULL, FULL,
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#.....i........................#",
        "#.........................i....#",
        "#....................#.........#",
        "#....................#.........#",
        "#.P.......^^.........#.....E...#",
        FULL, FULL] },
    { name: "Two Candles", ghosts: 2,
      hint: "Door A needs both plates held. The lever opens the gate for a moment: run for it.",
      levers: [{ link: "c", time: 80 }],
      map: [FULL, FULL, FULL, FULL,
        "#....#.........................#",
        "#....#.........................#",
        "#....#.....i...................#",
        "#....#....a....................#",
        "#....#...----..................#",
        "#....#.........................#",
        "#....#.......----..............#",
        "#....#.........................#",
        "#....#...----..................#",
        "#....C..................########",
        "#....C.......----.......A......#",
        "#.a..C.^^..!.......P....A...E..#",
        FULL, FULL] },
    { name: "The Bell Tower", ghosts: 2,
      hint: "Ghosts ride lifts too. One holds the plate, one waits up top to be your step.",
      lifts: [{ c: 6, w: 2, y: 480, y2: 210, link: "b" }],
      map: [FULL,
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..................#...........#",
        "#..................#.....i.....#",
        "#..................#.......E...#",
        "#.......########################",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#....i..............i..........#",
        "#..............................#",
        "#..............................#",
        "#..P.........^^.........b......#",
        "######..########################",
        FULL] },
    { name: "Ladder of Souls", ghosts: 2,
      hint: "Stack the dead: your second ghost stands on the first one's head.",
      map: [FULL, FULL,
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#.....i...............i........#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#.....................#........#",
        "#.....................#........#",
        "#.....................#........#",
        "#.P...................#....E...#",
        "#######....#####################",
        "#######....#####################"] },
    { name: "Choir", ghosts: 3,
      hint: "Three plates, three spirits. The way out is at the top of the stair.",
      levers: [{ link: "c", time: 85 }],
      map: [FULL,
        "#....#...............A.........#",
        "#....#...............A.........#",
        "#....#..a...i........A....E....#",
        "#....##########----#############",
        "#....#.........................#",
        "#....#.........----............#",
        "#....#.........................#",
        "#....#.....----................#",
        "#....#.........................#",
        "#....#.........----............#",
        "#....#.........................#",
        "#....#.....----................#",
        "#....C.........................#",
        "#....C.........----............#",
        "#.a..C.^^..!.P...............a.#",
        "#####################....#######",
        "#####################....#######"] },
    { name: "Stair of Souls", ghosts: 3,
      hint: "Five stones high. Three of you, one on top of another, make a stair.",
      map: [FULL,
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#.......i.............i........#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#....................#.........#",
        "#....................#.........#",
        "#....................#.........#",
        "#....................#.........#",
        "#.P.....^^^..........#....E....#",
        "#############...################",
        "#############...################"] },
    { name: "Last Rites", ghosts: 3,
      hint: "Everything you have learned. One for the plate, one for the step, one for the door.",
      lifts: [{ c: 6, w: 2, y: 480, y2: 210, link: "b" }],
      levers: [{ link: "c", time: 70 }],
      map: [FULL,
        "#.......................A......#",
        "#.......................A......#",
        "#.......................A......#",
        "#.............#.........A......#",
        "#.............#.........A......#",
        "#.............#...a.....A..E...#",
        "#.......########################",
        "#........................#.....#",
        "#........................#.....#",
        "#........................#.....#",
        "#........................#.....#",
        "#....i............i......#.....#",
        "#........................C.....#",
        "#........................C.....#",
        "#..P.......^^.......!....C..b..#",
        "######..########################",
        FULL] }
  ];

  // ---- set up -----------------------------------------------------------------------------------------------------
  var canvas = document.getElementById("game");
  var ctx = canvas.getContext("2d");
  canvas.width = W; canvas.height = H;
  var splash = document.getElementById("splash");
  var staticLayer = document.createElement("canvas");
  staticLayer.width = W; staticLayer.height = H;
  var shot = location.hash === "#shot";
  var started = false, manual = false;

  var save = (function () {
    var s = {};
    try { s = JSON.parse(localStorage.getItem("seanceSave") || "{}") || {}; } catch (e) { s = {}; }
    s.unlocked = s.unlocked || 0; s.best = s.best || {};
    return s;
  })();
  function writeSave() { if (shot || manual) return; try { localStorage.setItem("seanceSave", JSON.stringify(save)); } catch (e) {} }

  var level = 0, L, ghosts = [], run = [], t = 0, player, state = "splash", stateT = 0, paused = false;
  var flare = 0, particles = [], fog = [], note = "", noteT = 0, planX = -1, rewindSummon = false, rewindRun = null;

  function parse(idx) {
    var def = LEVELS[idx], lv = 0;
    var L = { def: def, grid: [], plates: [], doors: [], levers: [], lifts: [], candles: [], spikes: [], start: null, exit: null };
    for (var r = 0; r < ROWS; r++) {
      var row = def.map[r] || FULL;
      if (row.length !== COLS) console.error("SÉANCE level " + (idx + 1) + " row " + r + " is " + row.length + " wide");
      L.grid.push([]);
      for (var c = 0; c < COLS; c++) {
        var ch = row.charAt(c) || "#", cell = ".";
        if (ch === "#" || ch === "-") cell = ch;
        else if (ch === "^") L.spikes.push({ x: c * T + 4, y: r * T + 15, w: T - 8, h: 15, c: c, r: r });
        else if (ch === "P") L.start = { x: c * T + (T - PW) / 2, y: r * T + T - PH };
        else if (ch === "E") L.exit = { x: c * T + 6, y: r * T, w: T - 12, h: T, c: c, r: r };
        else if (ch === "i") L.candles.push({ x: c * T + T / 2, y: r * T + T, ph: c * 7.3 + r * 3.1 });
        else if (ch === "!") { var d = def.levers[lv++]; L.levers.push({ x: c * T, y: r * T, w: T, h: T, link: d.link, time: d.time, timer: 0 }); }
        else if (ch >= "a" && ch <= "e") L.plates.push({ x: c * T + 3, y: r * T + T - 6, w: T - 6, h: 6, link: ch, pressed: false });
        else if (ch >= "A" && ch <= "E") cell = ch;
        L.grid[r].push(cell);
      }
    }
    for (c = 0; c < COLS; c++) {                          // doors: each vertical run of one letter is one door
      for (r = 0; r < ROWS; r++) {
        var g = L.grid[r][c];
        if (g >= "A" && g <= "E" && (r === 0 || L.grid[r - 1][c] !== g)) {
          var n = 0;
          while (r + n < ROWS && L.grid[r + n][c] === g) n++;
          L.doors.push({ x: c * T, y: r * T, w: T, h: n * T, link: g.toLowerCase(), open: 0, wasOpen: false });
        }
      }
    }
    for (r = 0; r < ROWS; r++) for (c = 0; c < COLS; c++) if (L.grid[r][c] >= "A" && L.grid[r][c] <= "E") L.grid[r][c] = ".";
    (def.lifts || []).forEach(function (l) {
      L.lifts.push({ x: l.c * T, w: l.w * T, h: 12, y0: l.y, y1: l.y2, y: l.y, link: l.link, speed: l.speed || 1.6, dy: 0 });
    });
    return L;
  }

  function loadLevel(i) {
    level = Math.max(0, Math.min(LEVELS.length - 1, i));
    L = parse(level);
    ghosts = [];
    buildStatic();
    startAttempt();
    state = "play"; stateT = 0; paused = false;
    note = ROMAN[level] + " · " + L.def.name.toUpperCase(); noteT = 150;
  }

  function startAttempt() {
    t = 0; run = [];
    player = { x: L.start.x, y: L.start.y, vx: 0, vy: 0, ground: null, face: 1, coyote: 0, buffer: 0, prevJ: false,
               jumping: false, alive: true };
    L.plates.forEach(function (p) { p.pressed = false; });
    L.levers.forEach(function (l) { l.timer = 0; });
    L.doors.forEach(function (d) { d.open = 0; d.wasOpen = false; });
    L.lifts.forEach(function (l) { l.y = l.y0; l.dy = 0; });
    ghosts.forEach(function (g) { g.on = false; g.fade = 0; g.x = g.rec[0].x; g.y = g.rec[0].y; g.dx = g.dy = 0; });
  }

  // ---- the simulation: one fixed step ------------------------------------------------------------------------------
  function overlap(a, b) { return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y; }
  function box(b) { return { x: b.x, y: b.y, w: PW, h: PH }; }
  function active(link) {
    var any = false, all = true;
    L.plates.forEach(function (p) { if (p.link === link) { any = true; if (!p.pressed) all = false; } });
    L.levers.forEach(function (l) { if (l.link === link) { any = true; if (l.timer <= 0) all = false; } });
    return any && all;
  }
  function doorRect(d, open) { return { x: d.x, y: d.y, w: d.w, h: d.h * (1 - open) }; }

  function solidsAt(rect) {
    var out = [];
    var c0 = Math.floor(rect.x / T), c1 = Math.floor((rect.x + rect.w - 0.001) / T);
    var r0 = Math.floor(rect.y / T), r1 = Math.floor((rect.y + rect.h - 0.001) / T);
    for (var r = r0; r <= r1; r++) for (var c = c0; c <= c1; c++) {
      if (r >= ROWS) continue;                             // below the map: the pit, nothing to stand on
      if (c < 0 || c >= COLS || r < 0 || L.grid[r][c] === "#") out.push({ x: c * T, y: r * T, w: T, h: T });
    }
    L.doors.forEach(function (d) {
      var dr = doorRect(d, d.open);
      if (dr.h > 0.5 && overlap(rect, dr)) out.push(dr);
    });
    return out;
  }

  // things you can land on from above: ledges, lifts and the heads of your ghosts
  function oneWays() {
    var out = [];
    L.lifts.forEach(function (l) { out.push({ x: l.x, y: l.y, w: l.w, dx: 0, dy: l.dy, ref: l }); });
    ghosts.forEach(function (g) { if (g.on) out.push({ x: g.x, y: g.y, w: PW, dx: g.dx, dy: g.dy, ref: g }); });
    return out;
  }

  function simFrame(bits) {
    var i, g;
    for (i = 0; i < ghosts.length; i++) {                // 1. the ghosts move to where they were at this moment
      g = ghosts[i];
      var was = g.on;
      g.on = t < g.rec.length;
      if (g.on) {
        var s = g.rec[t], px = g.x, py = g.y;
        g.x = s.x; g.y = s.y; g.face = s.f; g.air = s.a;
        g.dx = t > 0 ? g.x - px : 0; g.dy = t > 0 ? g.y - py : 0;
      } else {
        g.dx = g.dy = 0;
        if (was) { g.fade = 1; burst(g.x + PW / 2, g.y + PH / 2, 24, 2, [C.sky, C.ghost]); }
      }
    }
    var bodies = [];
    ghosts.forEach(function (g) { if (g.on) bodies.push(box(g)); });
    if (player.alive) bodies.push(box(player));

    L.plates.forEach(function (p) {                       // 2. plates, levers, doors and lifts
      var hit = { x: p.x, y: p.y - 3, w: p.w, h: p.h + 3 }, on = bodies.some(function (b) { return overlap(b, hit); });
      if (on !== p.pressed) { if (!manual) Sfx.plate(on); if (on) burst(p.x + p.w / 2, p.y, 8, 1.5, [C.green]); }
      p.pressed = on;
    });
    L.levers.forEach(function (l) {
      var on = bodies.some(function (b) { return overlap(b, l); });
      if (on && l.timer <= 0 && !manual) Sfx.lever();
      if (on) l.timer = l.time; else if (l.timer > 0) l.timer--;
    });
    L.doors.forEach(function (d) {
      if (active(d.link)) d.open = Math.min(1, d.open + 0.1);
      else {
        var n = Math.max(0, d.open - 0.05);
        if (!(player.alive && overlap(box(player), doorRect(d, n)))) d.open = n;   // a door never shuts on the living
      }
      var isOpen = d.open > 0.5;
      if (isOpen !== d.wasOpen && !manual) Sfx.door();
      d.wasOpen = isOpen;
    });
    L.lifts.forEach(function (l) {
      var target = active(l.link) ? l.y1 : l.y0;
      l.dy = Math.max(-l.speed, Math.min(l.speed, target - l.y));
      l.y += l.dy;
    });

    if (player.alive) physics(bits);                     // 3. the living
    run.push({ x: player.x, y: player.y, f: player.face, a: player.ground ? 0 : 1 });
    t++;

    if (player.alive) {                                   // 4. hazards and the way out
      var pb = box(player);
      var dead = player.y > H + 40 || L.spikes.some(function (s) { return overlap(pb, s); });
      if (dead) die();
      else if (overlap(pb, L.exit)) win();
    }
  }

  function physics(bits) {
    var p = player, L_ = bits & IN_L, R_ = bits & IN_R, J = !!(bits & IN_J);
    var carryDx = 0;
    if (p.ground && p.ground.ref) {                       // standing on a lift or a ghost: it carries you
      var gr = p.ground.ref;
      if (gr.on !== false) { carryDx = gr.dx || 0; p.y = (gr.rec ? gr.y : gr.y) - PH; }
    }
    var dir = (R_ ? 1 : 0) - (L_ ? 1 : 0);
    if (dir) p.face = dir;
    var onGround = !!p.ground, acc = onGround ? ACC_G : ACC_A, target = dir * RUN;
    if (p.vx < target) p.vx = Math.min(target, p.vx + acc); else if (p.vx > target) p.vx = Math.max(target, p.vx - acc);

    if (J && !p.prevJ) p.buffer = BUFFER; else if (p.buffer > 0) p.buffer--;
    p.prevJ = J;
    if (p.buffer > 0 && p.coyote > 0) {
      p.vy = JUMP; p.buffer = 0; p.coyote = 0; p.jumping = true; p.ground = null;
      if (!manual) Sfx.jump();
    }
    if (p.jumping && !J && p.vy < CUT) p.vy = CUT;
    if (p.vy >= 0) p.jumping = false;
    p.vy = Math.min(MAXFALL, p.vy + GRAV);

    var dx = p.vx + carryDx;                              // across, then down, each against the walls
    p.x += dx;
    solidsAt(box(p)).forEach(function (r) {
      if (dx > 0 || (dx === 0 && p.x + PW / 2 < r.x + r.w / 2)) p.x = Math.min(p.x, r.x - PW);
      else p.x = Math.max(p.x, r.x + r.w);
      p.vx = 0;
    });
    var prevBottom = p.y + PH, wasGround = !!p.ground;
    p.y += p.vy;
    p.ground = null;
    solidsAt(box(p)).forEach(function (r) {
      if (p.vy >= 0) { p.y = Math.min(p.y, r.y - PH); p.ground = { tile: true }; }
      else p.y = Math.max(p.y, r.y + r.h);
      p.vy = 0;
    });
    if (p.vy >= 0 && !p.ground) {
      var bottom = p.y + PH, best = null;
      var c0 = Math.floor(p.x / T), c1 = Math.floor((p.x + PW - 0.001) / T);
      for (var r = Math.floor(prevBottom / T) - 1; r <= Math.floor(bottom / T); r++) {
        if (r < 0 || r >= ROWS) continue;
        for (var c = c0; c <= c1; c++) if (c >= 0 && c < COLS && L.grid[r][c] === "-") {
          var top = r * T;
          if (prevBottom <= top + 0.5 && bottom >= top && (!best || top < best.y)) best = { y: top };
        }
      }
      oneWays().forEach(function (o) {
        var tol = 0.5 + Math.max(0, -o.dy) + Math.abs(o.dy);
        if (p.x < o.x + o.w && p.x + PW > o.x && prevBottom <= o.y + tol && bottom >= o.y && (!best || o.y < best.y)) best = o;
      });
      if (best) { p.y = best.y - PH; p.vy = 0; p.ground = best.ref ? { ref: best.ref } : { tile: true }; }
    }
    if (p.ground && !wasGround && !manual) { Sfx.land(); burst(p.x + PW / 2, p.y + PH, 5, 1, [C.pink, C.white]); }
    p.coyote = p.ground ? COYOTE : p.coyote - 1;
  }

  function die() {
    player.alive = false;
    state = "dead"; stateT = 0;
    burst(player.x + PW / 2, player.y + PH / 2, 60, 4, [C.pink, C.pink, C.white]);
    if (!manual) Sfx.die();
  }

  function win() {
    state = "won"; stateT = 0;
    burst(player.x + PW / 2, player.y + PH / 2, 90, 5, [C.pink, C.sky, C.white, C.ghost]);
    if (!manual) Sfx.win();
    save.unlocked = Math.max(save.unlocked, level + 1);
    var b = save.best[level];
    if (!b || t < b.t) save.best[level] = { t: t, g: ghosts.length };
    writeSave();
  }

  function beginRewind(summon) {
    if (state !== "play" && state !== "dead") return;
    if (summon && ghosts.length >= L.def.ghosts) {
      summon = false; note = "THE BOARD SAYS NO · no spirits left"; noteT = 120;
      if (!manual) Sfx.no();
    }
    if (summon && run.length < 20) summon = false;      // a blink of a run is not worth a spirit
    rewindSummon = summon; rewindRun = run;
    state = "rewind"; stateT = 0;
    if (!manual) Sfx.rewind();
  }

  function finishRewind() {
    if (rewindSummon) {
      ghosts.push({ rec: rewindRun, on: false, fade: 0, x: 0, y: 0, dx: 0, dy: 0, face: 1 });
      flare = 1;
      if (!manual) Sfx.summon();
    }
    startAttempt();
    if (rewindSummon) burst(player.x + PW / 2, player.y + PH / 2, 50, 3, [C.sky, C.ghost, C.white]);
    state = "play"; stateT = 0;
  }

  function banish() {
    if (state !== "play" && state !== "dead") return;
    ghosts = [];
    note = "THE SPIRITS ARE BANISHED"; noteT = 90;
    beginRewind(false);
  }

  function burst(x, y, n, speed, colours) {
    if (particles.length > 700) return;
    for (var i = 0; i < n; i++) {
      var a = Math.random() * Math.PI * 2, v = speed * (0.2 + Math.random() * 0.8), life = 30 + Math.random() * 40;
      particles.push({ x: x, y: y, vx: Math.cos(a) * v, vy: Math.sin(a) * v - 0.5, life: life, max: life,
                       c: colours[Math.floor(Math.random() * colours.length)] });
    }
  }

  // ---- input --------------------------------------------------------------------------------------------------------
  var keys = {}, touchKeys = {};
  function inputBits() {
    var b = 0;
    if (keys.arrowleft || keys.a || touchKeys.left) b |= IN_L;
    if (keys.arrowright || keys.d || touchKeys.right) b |= IN_R;
    if (keys.arrowup || keys.w || keys[" "] || keys.z || keys.k || touchKeys.jump) b |= IN_J;
    return b;
  }

  function startGame() {
    if (started) return;
    started = true;
    splash.classList.add("gone");
    document.body.classList.add("playing");
    var first = 0;
    while (first < LEVELS.length - 1 && save.best[first]) first++;
    loadLevel(first);
  }

  function anyKeyAdvance() {
    if (state === "won" && stateT > 45) {
      if (level + 1 < LEVELS.length) loadLevel(level + 1);
      else { state = "end"; stateT = 0; }
      return true;
    }
    if (state === "end" && stateT > 60) { loadLevel(0); return true; }
    return false;
  }

  function command(k) {
    if (k === "p") { if (state === "play" || state === "dead") paused = !paused; return true; }
    if (k === "m") { var m = Sfx.toggle(); note = m ? "SOUND OFF" : "SOUND ON"; noteT = 60; return true; }
    if (paused) return false;
    if (k === "r") { beginRewind(true); return true; }
    if (k === "t") { beginRewind(false); return true; }
    if (k === "x") { banish(); return true; }
    if (k === "[" || k === "pageup") { if (level > 0) loadLevel(level - 1); return true; }
    if (k === "]" || k === "pagedown") {
      if (level < save.unlocked && level + 1 < LEVELS.length) loadLevel(level + 1);
      else { note = "SOLVE THIS ONE FIRST"; noteT = 70; }
      return true;
    }
    return false;
  }

  document.addEventListener("keydown", function (e) {
    var k = e.key.toLowerCase();
    if (!started) { startGame(); e.preventDefault(); return; }
    if (anyKeyAdvance()) { e.preventDefault(); return; }
    if (!e.repeat && command(k)) { e.preventDefault(); return; }
    keys[k] = true;
    if (k.indexOf("arrow") === 0 || k === " ") e.preventDefault();
  });
  document.addEventListener("keyup", function (e) { keys[e.key.toLowerCase()] = false; });
  window.addEventListener("blur", function () { keys = {}; });
  splash.addEventListener("click", startGame);
  canvas.addEventListener("pointerdown", function () { if (!started) startGame(); else anyKeyAdvance(); });

  if ("ontouchstart" in window || navigator.maxTouchPoints > 0) document.body.classList.add("touch");
  Array.prototype.forEach.call(document.querySelectorAll("#touch button"), function (b) {
    var k = b.getAttribute("data-k");
    function down(e) {
      e.preventDefault();
      b.classList.add("down");
      if (anyKeyAdvance()) return;
      if (k === "summon") command("r"); else if (k === "retry") command("t"); else if (k === "pause") command("p");
      else touchKeys[k] = true;
    }
    function up(e) { e.preventDefault(); b.classList.remove("down"); touchKeys[k] = false; }
    b.addEventListener("pointerdown", down);
    b.addEventListener("pointerup", up);
    b.addEventListener("pointercancel", up);
    b.addEventListener("pointerleave", up);
  });

  // ---- drawing ------------------------------------------------------------------------------------------------------
  function glow(colour, blur) { ctx.shadowColor = colour; ctx.shadowBlur = blur; ctx.fillStyle = colour; ctx.strokeStyle = colour; }
  function noGlow() { ctx.shadowBlur = 0; }

  function buildStatic() {
    var s = staticLayer.getContext("2d");
    s.fillStyle = C.black; s.fillRect(0, 0, W, H);
    // tall arched windows far behind, with a few cold stars in them
    var rnd = seeded(level * 97 + 13);
    for (var i = 0; i < 5; i++) {
      var wx = 60 + i * 190 + rnd() * 40, wy = 70 + rnd() * 50, ww = 54, wh = 170;
      s.fillStyle = "#000c16";
      s.beginPath(); s.moveTo(wx, wy + wh); s.lineTo(wx, wy + ww / 2); s.arc(wx + ww / 2, wy + ww / 2, ww / 2, Math.PI, 0); s.lineTo(wx + ww, wy + wh); s.closePath(); s.fill();
      s.strokeStyle = C.navy; s.lineWidth = 3; s.stroke();
      s.fillStyle = C.deep;
      for (var k = 0; k < 6; k++) s.fillRect(wx + 6 + rnd() * (ww - 12), wy + 10 + rnd() * (wh - 20), 1.5, 1.5);
      s.fillStyle = C.navy; s.fillRect(wx + ww / 2 - 1, wy + 6, 2, wh - 6); s.fillRect(wx, wy + wh * 0.55, ww, 2);
    }
    for (var r = 0; r < ROWS; r++) for (var c = 0; c < COLS; c++) {
      var g = L.grid[r][c], x = c * T, y = r * T;
      if (g === "#") {
        s.fillStyle = C.navy; s.fillRect(x, y, T, T);
        s.fillStyle = "#00213a";                        // stone seams
        var off = (r % 2) * 15;
        s.fillRect(x, y + 14, T, 1);
        s.fillRect(x + ((off + 7) % T), y, 1, 14);
        s.fillRect(x + ((off + 22) % T), y + 15, 1, 15);
        s.fillStyle = C.deep;                           // edges that face open air
        if (!isWall(r - 1, c)) { s.fillStyle = C.blue; s.fillRect(x, y, T, 2); }
        s.fillStyle = C.deep;
        if (!isWall(r + 1, c) && r + 1 < ROWS) s.fillRect(x, y + T - 1, T, 1);
        if (!isWall(r, c - 1)) s.fillRect(x, y, 1, T);
        if (!isWall(r, c + 1)) s.fillRect(x + T - 1, y, 1, T);
      } else if (g === "-") {
        s.fillStyle = C.deep; s.fillRect(x, y, T, 7);
        s.fillStyle = C.sky; s.fillRect(x, y, T, 2);
        s.fillStyle = C.navy; s.fillRect(x + 4, y + 7, 2, 4); s.fillRect(x + T - 6, y + 7, 2, 4);
      }
    }
    s.shadowColor = C.sky; s.shadowBlur = 8;
    L.spikes.forEach(function (sp) {
      var x = sp.c * T, y = sp.r * T;
      s.fillStyle = "#bfe3f5";
      for (var k = 0; k < 3; k++) {
        s.beginPath(); s.moveTo(x + k * 10, y + T); s.lineTo(x + k * 10 + 5, y + 12); s.lineTo(x + k * 10 + 10, y + T); s.closePath(); s.fill();
      }
    });
    s.shadowBlur = 0;
  }
  function isWall(r, c) { return r < 0 || c < 0 || c >= COLS || (r < ROWS && L.grid[r][c] === "#"); }
  function seeded(n) { return function () { n = (n * 9301 + 49297) % 233280; return n / 233280; }; }

  function drawFigure(x, y, face, ghost, alpha, now) {
    ctx.save();
    ctx.globalAlpha = alpha;
    if (ghost) {
      glow(C.sky, 18);
      ctx.fillStyle = C.ghost;
      ctx.beginPath();                                     // a sheet with a wavy hem
      ctx.moveTo(x, y + PH);
      ctx.lineTo(x, y + 8);
      ctx.arc(x + PW / 2, y + 9, PW / 2, Math.PI, 0);
      ctx.lineTo(x + PW, y + PH);
      for (var k = 3; k >= 0; k--) ctx.lineTo(x + k * PW / 4 + PW / 8, y + PH - 4 + 3 * Math.sin(now / 120 + k * 2));
      ctx.closePath(); ctx.fill();
      noGlow();
      ctx.fillStyle = C.deep;
      ctx.fillRect(x + PW / 2 + face * 3 - 3, y + 8, 3, 4); ctx.fillRect(x + PW / 2 + face * 3 + 2, y + 8, 3, 4);
    } else {
      glow(C.pink, 14);
      ctx.beginPath(); ctx.arc(x + PW / 2, y + 7, 7, 0, Math.PI * 2); ctx.fill();       // head
      ctx.fillRect(x + 2, y + 12, PW - 4, PH - 12);                                      // body
      noGlow();
      ctx.fillStyle = C.black;
      ctx.fillRect(x + PW / 2 + face * 2 - 2, y + 5, 2, 3); ctx.fillRect(x + PW / 2 + face * 2 + 2, y + 5, 2, 3);
    }
    ctx.restore();
  }

  function drawCandle(cd, now) {
    var fl = Math.sin(now / 90 + cd.ph) * 0.5 + Math.sin(now / 37 + cd.ph * 2) * 0.3;
    var big = 1 + flare * 2.2, x = cd.x, y = cd.y;
    ctx.fillStyle = "#d9d4e8"; ctx.fillRect(x - 3, y - 14, 6, 14);                       // the wax
    ctx.fillStyle = C.navy; ctx.fillRect(x - 6, y - 2, 12, 2);
    var col = flare > 0.05 ? C.sky : C.pink;
    glow(col, 20 + 25 * flare);
    ctx.beginPath();
    ctx.ellipse(x + fl, y - 19 - 3 * big, 3 * big, (6 + fl) * big, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = C.white; noGlow();
    ctx.beginPath(); ctx.ellipse(x + fl * 0.5, y - 18 - 2 * big, 1.2 * big, 3 * big, 0, 0, Math.PI * 2); ctx.fill();
    ctx.globalAlpha = 0.07 + 0.05 * fl + 0.18 * flare;                                 // a pool of light
    ctx.fillStyle = col;
    ctx.beginPath(); ctx.arc(x, y - 20, 40 + 50 * flare, 0, Math.PI * 2); ctx.fill();
    ctx.globalAlpha = 1;
  }

  function drawWorld(now, ghostFrame) {
    ctx.drawImage(staticLayer, 0, 0);
    // fog drifting through
    ctx.fillStyle = C.navy;
    fog.forEach(function (f) {
      f.x += f.v; if (f.x > W + 120) f.x = -120;
      ctx.globalAlpha = 0.18; ctx.beginPath(); ctx.arc(f.x, f.y, f.r, 0, Math.PI * 2); ctx.fill();
    });
    ctx.globalAlpha = 1;
    L.candles.forEach(function (cd) { drawCandle(cd, now); });

    var e = L.exit;                                     // the way out: an arch of pink light
    var ex = e.c * T + T / 2, ey = e.r * T + T;
    glow(C.pink, 22 + 8 * Math.sin(now / 300));
    ctx.lineWidth = 3;
    ctx.beginPath(); ctx.moveTo(ex - 13, ey); ctx.lineTo(ex - 13, ey - 34); ctx.arc(ex, ey - 34, 13, Math.PI, 0); ctx.lineTo(ex + 13, ey); ctx.stroke();
    noGlow();
    ctx.fillStyle = "#2a0a2a"; ctx.globalAlpha = 0.7;
    ctx.beginPath(); ctx.moveTo(ex - 11, ey); ctx.lineTo(ex - 11, ey - 34); ctx.arc(ex, ey - 34, 11, Math.PI, 0); ctx.lineTo(ex + 11, ey); ctx.fill();
    ctx.globalAlpha = 1;
    for (var m = 0; m < 4; m++) {
      var a = now / 600 + m * 1.57;
      ctx.fillStyle = C.pink; ctx.fillRect(ex + Math.cos(a) * 6 - 1, ey - 22 + Math.sin(a * 1.3) * 14, 2, 2);
    }

    L.plates.forEach(function (p) {
      if (p.pressed) { glow(C.green, 16); ctx.fillRect(p.x, p.y + 3, p.w, 3); }
      else { noGlow(); ctx.fillStyle = C.deep; ctx.fillRect(p.x, p.y, p.w, p.h); ctx.fillStyle = C.green; ctx.globalAlpha = 0.5; ctx.fillRect(p.x, p.y, p.w, 1); ctx.globalAlpha = 1; }
      noGlow();
      ctx.fillStyle = p.pressed ? C.green : C.blue; ctx.font = "11px Georgia"; ctx.textAlign = "center";
      ctx.fillText(p.link.toUpperCase(), p.x + p.w / 2, p.y - 8);
    });
    L.levers.forEach(function (l) {
      var on = l.timer > 0, bx = l.x + T / 2, by = l.y + T;
      ctx.fillStyle = C.deep; ctx.fillRect(bx - 8, by - 5, 16, 5);
      ctx.strokeStyle = on ? C.green : C.sky; ctx.lineWidth = 3;
      if (on) glow(C.green, 12);
      ctx.beginPath(); ctx.moveTo(bx, by - 4); ctx.lineTo(bx + (on ? 9 : -9), by - 20); ctx.stroke();
      ctx.beginPath(); ctx.arc(bx + (on ? 9 : -9), by - 21, 3.5, 0, Math.PI * 2); ctx.fillStyle = on ? C.green : C.sky; ctx.fill();
      noGlow();
      if (on) {
        ctx.strokeStyle = C.green; ctx.lineWidth = 2;
        ctx.beginPath(); ctx.arc(bx, by - 14, 15, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * l.timer / l.time); ctx.stroke();
      }
      ctx.fillStyle = on ? C.green : C.blue; ctx.font = "11px Georgia"; ctx.textAlign = "center";
      ctx.fillText(l.link.toUpperCase(), bx, l.y - 2);
    });
    L.doors.forEach(function (d) {
      var r = doorRect(d, d.open);
      var lit = active(d.link);
      if (r.h > 0.5) {
        ctx.fillStyle = C.navy; ctx.fillRect(r.x + 2, r.y, r.w - 4, r.h);
        ctx.fillStyle = lit ? C.green : C.deep;
        for (var k = 0; k < 4; k++) ctx.fillRect(r.x + 4 + k * 7, r.y, 2, r.h);
        glow(lit ? C.green : C.sky, 10);
        ctx.fillRect(r.x + 2, r.y + r.h - 3, r.w - 4, 3);
        noGlow();
      }
      ctx.fillStyle = lit ? C.green : C.sky; ctx.font = "bold 12px Georgia"; ctx.textAlign = "center";
      ctx.fillText(d.link.toUpperCase(), d.x + T / 2, d.y + d.h - 6 - (r.h > 20 ? 0 : 0));
    });
    L.lifts.forEach(function (l) {
      ctx.strokeStyle = C.deep; ctx.lineWidth = 1; ctx.setLineDash([3, 4]);
      ctx.beginPath(); ctx.moveTo(l.x + 6, l.y1 - 30); ctx.lineTo(l.x + 6, l.y); ctx.moveTo(l.x + l.w - 6, l.y1 - 30); ctx.lineTo(l.x + l.w - 6, l.y); ctx.stroke();
      ctx.setLineDash([]);
      var lit = active(l.link);
      ctx.fillStyle = C.deep; ctx.fillRect(l.x, l.y, l.w, l.h);
      glow(lit ? C.green : C.sky, 12); ctx.fillRect(l.x, l.y, l.w, 2); noGlow();
      ctx.fillStyle = lit ? C.green : C.sky; ctx.font = "10px Georgia"; ctx.textAlign = "center";
      ctx.fillText(l.link.toUpperCase(), l.x + l.w / 2, l.y + 11);
    });

    ghosts.forEach(function (g, i) {                     // the dead, with a faint trail behind
      var f = ghostFrame === undefined ? t - 1 : ghostFrame;
      if (f < 0) f = 0;
      if (f < g.rec.length) {
        for (var k = 3; k >= 1; k--) {
          var s = g.rec[Math.max(0, f - k * 5)];
          drawFigure(s.x, s.y, s.f, true, 0.08 * (4 - k), now);
        }
        var cur = g.rec[f];
        drawFigure(cur.x, cur.y, cur.f, true, 0.62 + 0.08 * Math.sin(now / 200 + i), now);
        ctx.fillStyle = C.sky; ctx.font = "10px Georgia"; ctx.textAlign = "center";
        ctx.fillText(ROMAN[i], cur.x + PW / 2, cur.y - 5);
      } else if (g.fade > 0 && ghostFrame === undefined) {
        var last = g.rec[g.rec.length - 1];
        drawFigure(last.x, last.y - (1 - g.fade) * 20, last.f, true, g.fade * 0.6, now);
      }
    });
  }

  function drawOuija(now) {
    var max = L.def.ghosts, left = max - ghosts.length;
    var bw = 250, bh = 66, bx = W - bw - 12, by = 10;
    ctx.fillStyle = C.navy; ctx.strokeStyle = C.deep; ctx.lineWidth = 2;
    roundRect(bx, by, bw, bh, 12); ctx.fill(); ctx.stroke();
    ctx.font = "11px Georgia"; ctx.textAlign = "center"; ctx.fillStyle = C.sky;
    ctx.fillText("YES", bx + 24, by + 18); ctx.fillText("NO", bx + bw - 22, by + 18);
    ctx.fillText("☾", bx + 24, by + 34); ctx.fillText("☼", bx + bw - 22, by + 34);
    ctx.fillStyle = C.blue; ctx.font = "9px Georgia"; ctx.fillText("SPIRITS LEFT", bx + bw / 2, by + 14);
    var xs = [];
    for (var i = 0; i <= max; i++) {
      var x = bx + 60 + (max ? i * (bw - 120) / max : (bw - 120) / 2), y = by + 44 - Math.sin(Math.PI * (max ? i / max : 0.5)) * 12;
      xs.push([x, y]);
      ctx.fillStyle = i === left ? C.pink : C.sky; ctx.font = "18px Georgia";
      ctx.fillText(String(i), x, y + 6);
    }
    var target = xs[left];
    if (planX < 0) planX = target[0];
    planX += (target[0] - planX) * (manual ? 1 : 0.12);
    var py = target[1] + 3;
    // the planchette: a heart-shaped pointer with a window over the number
    glow(C.pink, 12); ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(planX, py - 22);
    ctx.bezierCurveTo(planX + 22, py - 18, planX + 20, py + 12, planX, py + 16);
    ctx.bezierCurveTo(planX - 20, py + 12, planX - 22, py - 18, planX, py - 22);
    ctx.stroke(); noGlow();
    ctx.beginPath(); ctx.arc(planX, py - 3, 10, 0, Math.PI * 2); ctx.strokeStyle = C.pink; ctx.lineWidth = 1.5; ctx.stroke();
  }

  function roundRect(x, y, w, h, r) {
    ctx.beginPath(); ctx.moveTo(x + r, y); ctx.arcTo(x + w, y, x + w, y + h, r); ctx.arcTo(x + w, y + h, x, y + h, r);
    ctx.arcTo(x, y + h, x, y, r); ctx.arcTo(x, y, x + w, y, r); ctx.closePath();
  }

  function fmt(frames) {
    var s = frames / 60, m = Math.floor(s / 60);
    return m + ":" + ("0" + (s - m * 60).toFixed(1)).slice(-4);
  }

  function centreText(text, y, size, colour, blur) {
    ctx.textAlign = "center"; ctx.font = size + "px Georgia";
    glow(colour, blur || 0); ctx.fillText(text, W / 2, y); noGlow();
  }

  function draw(now) {
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    if (!L) { ctx.fillStyle = C.black; ctx.fillRect(0, 0, W, H); return; }
    var rewinding = state === "rewind";
    var k = rewinding ? stateT / 40 : 0;
    var rf = rewinding ? Math.floor((rewindRun.length - 1) * (1 - k)) : undefined;
    drawWorld(now, rf);

    if (rewinding) {
      var s = rewindRun[Math.max(0, rf)];
      if (s) drawFigure(s.x, s.y, s.f, false, 0.8, now);
    } else if (player.alive) {
      drawFigure(player.x, player.y, player.face, false, 1, now);
    }

    particles.forEach(function (p) {
      ctx.globalAlpha = p.life / p.max;
      glow(p.c, 8);
      ctx.fillRect(p.x - 1.5, p.y - 1.5, 3, 3);
    });
    ctx.globalAlpha = 1; noGlow();

    if (rewinding) {                                     // the tape running back
      ctx.fillStyle = C.sky; ctx.globalAlpha = 0.12;
      for (var y = (now / 4) % 12; y < H; y += 12) ctx.fillRect(0, y, W, 2);
      ctx.globalAlpha = 0.25; ctx.fillRect(0, (1 - k) * H, W, 6);
      ctx.globalAlpha = 1;
      centreText(rewindSummon ? "◀◀  SUMMONING" : "◀◀  REWIND", H / 2, 40, rewindSummon ? C.sky : C.pink, 20);
    }

    // HUD
    ctx.textAlign = "left"; ctx.font = "18px Georgia";
    glow(C.pink, 8); ctx.fillText(ROMAN[level] + " · " + L.def.name.toUpperCase(), 14, 28); noGlow();
    ctx.font = "14px Georgia"; ctx.fillStyle = C.sky;
    var b = save.best[level];
    ctx.fillText("Time " + fmt(t) + (b ? "    Best " + fmt(b.t) : "") + (Sfx.isMuted() ? "    ♪ off" : ""), 14, 50);
    drawOuija(now);
    ctx.textAlign = "center"; ctx.font = "italic 14px Georgia"; ctx.fillStyle = "#c8e8f8";
    ctx.globalAlpha = 0.85;
    ctx.fillText(L.def.hint, W / 2, H - 12);
    ctx.globalAlpha = 0.6; ctx.font = "11px Georgia"; ctx.textAlign = "left"; ctx.fillStyle = C.sky;
    ctx.fillText("←→ move  ↑/Space jump  R summon  T retry  X banish  [ ] level  P pause  M mute", 14, H - 30 + 0);
    ctx.globalAlpha = 1;

    if (noteT > 0) { ctx.globalAlpha = Math.min(1, noteT / 30); centreText(note, 110, 26, C.pink, 16); ctx.globalAlpha = 1; }

    var dim = function () { ctx.fillStyle = C.black; ctx.globalAlpha = 0.6; ctx.fillRect(0, 0, W, H); ctx.globalAlpha = 1; };
    if (paused) { dim(); centreText("PAUSED", H / 2 - 10, 56, C.pink, 24); centreText("P to carry on", H / 2 + 30, 18, C.white); }
    else if (state === "dead" && stateT > 20) {
      centreText("YOU DIED", H / 2 - 20, 48, C.pink, 24);
      centreText(ghosts.length < L.def.ghosts ? "R  summon this run as a ghost     T  try again" : "T  try again     X  banish your ghosts", H / 2 + 18, 18, C.white);
    } else if (state === "won" && stateT > 30) {
      dim();
      centreText("THE SPIRITS ARE AT REST", H / 2 - 30, 40, C.pink, 24);
      centreText("Solved in " + fmt(t) + " with " + ghosts.length + (ghosts.length === 1 ? " ghost" : " ghosts"), H / 2 + 10, 20, C.sky);
      centreText(level + 1 < LEVELS.length ? "Any key for the next room" : "Any key", H / 2 + 44, 16, C.white);
    } else if (state === "end") {
      dim();
      var total = 0; for (var i = 0; i < LEVELS.length; i++) if (save.best[i]) total += save.best[i].t;
      centreText("THE SÉANCE IS OVER", H / 2 - 30, 46, C.pink, 26);
      centreText("Every room solved. Best total " + fmt(total), H / 2 + 12, 20, C.sky);
      centreText("Any key to begin again", H / 2 + 46, 16, C.white);
    }
  }

  // ---- the loop -----------------------------------------------------------------------------------------------------
  function step() {
    if (paused) return;
    if (state === "play") simFrame(inputBits());
    else if (state === "rewind") { stateT++; if (stateT >= 40) finishRewind(); }
    else if (state !== "splash") stateT++;
    tick();
  }

  function tick() {                                       // the things that fade and fly, one fixed step
    if (flare > 0) flare = Math.max(0, flare - 0.008);
    if (noteT > 0) noteT--;
    ghosts.forEach(function (g) { if (g.fade > 0) g.fade = Math.max(0, g.fade - 0.03); });
    for (var i = particles.length - 1; i >= 0; i--) {
      var p = particles[i];
      p.x += p.vx; p.y += p.vy; p.vx *= 0.96; p.vy = p.vy * 0.96 + 0.05;
      if (--p.life <= 0) particles.splice(i, 1);
    }
  }

  var acc = 0, last = 0;
  function loop(now) {
    if (!last) last = now;
    acc += Math.min(100, now - last); last = now;
    while (acc >= STEP) { acc -= STEP; if (!manual) step(); }
    draw(now);
    requestAnimationFrame(loop);
  }

  for (var f = 0; f < 9; f++) fog.push({ x: Math.random() * W, y: 120 + Math.random() * 380, r: 60 + Math.random() * 90, v: 0.1 + Math.random() * 0.25 });

  // a small window for the tests and the README screenshots: runs frames by hand with given inputs
  window.SEANCE = {
    levels: LEVELS.length,
    begin: function () { manual = true; startGame(); },
    load: function (i) { manual = true; if (!started) startGame(); loadLevel(i); },
    frame: function (bits) { if (state === "play") simFrame(bits || 0); tick(); },
    summon: function () { beginRewind(true); if (state === "rewind") finishRewind(); },
    retry: function () { beginRewind(false); if (state === "rewind") finishRewind(); },
    info: function () {
      return { state: state, t: t, x: player.x, y: player.y, vx: player.vx, vy: player.vy, ground: !!player.ground,
               alive: player.alive, ghosts: ghosts.length, level: level,
               doors: L.doors.map(function (d) { return d.open; }), lifts: L.lifts.map(function (l) { return l.y; }) };
    },
    draw: function (now) { step(); draw(now || 1000); },
    pose: function (st, n) { state = st; stateT = n || 0; },
    setRewind: function (summon, k) { rewindSummon = summon; rewindRun = run; state = "rewind"; stateT = k; },
    flare: function (v) { flare = v; },
    live: function () { manual = false; }
  };

  if (shot) { splash.style.display = "none"; }
  requestAnimationFrame(loop);
})();
