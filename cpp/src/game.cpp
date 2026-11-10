// game.cpp - SÉANCE, ported from the browser game (site/game.js) rule for rule.
// A side-view platformer where you summon your past. Run the level, then rewind: time goes back to the start of the
// attempt and the run you just made rises as a ghost that repeats it frame by frame. Your ghosts hold plates, ride
// lifts and lend you their heads to stand on. The physics runs at a fixed 60 steps a second, so every replay is exact.
#include "game.h"
#include "synth.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr float W = 960, H = 540, T = 30;
constexpr int COLS = 32, ROWS = 18;
constexpr double PW = 18, PH = 26;                         // the living and the dead are the same size
constexpr double GRAV = 0.45, JUMP = -9, RUN = 3.2, ACC_G = 0.6, ACC_A = 0.4, MAXFALL = 10, CUT = -3.5;
constexpr int COYOTE = 6, BUFFER = 6;
constexpr float PI = 3.14159265358979f;
using Game::IN_L; using Game::IN_R; using Game::IN_J;

namespace C {
const ImU32 navy = rgb("#001725"), deep = rgb("#005184"), blue = rgb("#007ab7"), sky = rgb("#34a9e3"),
            pink = rgb("#f088e9"), green = rgb("#39ff6a"), black = rgb("#020611"), ghost = rgb("#cfeeff"),
            white = rgb("#ffffff");
}
const char* ROMAN[] = {"I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"};

// ---- the levels -------------------------------------------------------------------------------------------------------
// # wall   - ledge you can jump up through   ^ spikes   P start   E the way out   i a candle
// a-e pressure plates   A-E doors (open while every plate / lever with the same letter is held)   ! a lever
// (a lever keeps its door open for a while after you leave it). Lifts rise to y2 while their letter is held.
const char* FULL = "################################";
struct LiftDef { int c, w; double y, y2; char link; };
struct LeverDef { char link; int time; };
struct LevelDef {
    const char* name; int ghosts; const char* hint;
    std::vector<LiftDef> lifts; std::vector<LeverDef> levers; std::vector<const char*> map;
};
const std::vector<LevelDef> LEVELS = {
    {"First Light", 1, "Stand on the plate and press R. Your past self will hold the door while you walk through.", {}, {},
     {FULL, FULL, FULL, FULL,
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
      FULL, FULL}},
    {"Ascension", 1, "A lift rises while its plate is held. Leave a ghost on the plate, then ride.",
     {{4, 2, 480, 120, 'b'}}, {},
     {FULL,
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
      FULL}},
    {"Stand on Me", 1, "The living can stand on the dead. Leave a ghost by the wall and climb.", {}, {},
     {FULL, FULL, FULL, FULL, FULL,
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
      FULL, FULL}},
    {"Two Candles", 2, "Door A needs both plates held. The lever opens the gate for a moment: run for it.",
     {}, {{'c', 80}},
     {FULL, FULL, FULL, FULL,
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
      FULL, FULL}},
    {"The Bell Tower", 2, "Ghosts ride lifts too. One holds the plate, one waits up top to be your step.",
     {{6, 2, 480, 210, 'b'}}, {},
     {FULL,
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
      FULL}},
    {"Ladder of Souls", 2, "Stack the dead: your second ghost stands on the first one's head.", {}, {},
     {FULL, FULL,
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
      "#######....#####################"}},
    {"Choir", 3, "Three plates, three spirits. The way out is at the top of the stair.", {}, {{'c', 85}},
     {FULL,
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
      "#####################....#######"}},
    {"Stair of Souls", 3, "Five stones high. Three of you, one on top of another, make a stair.", {}, {},
     {FULL,
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
      "#############...################"}},
    {"Last Rites", 3, "Everything you have learned. One for the plate, one for the step, one for the door.",
     {{6, 2, 480, 210, 'b'}}, {{'c', 70}},
     {FULL,
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
      FULL}},
};
const int NLEVELS = (int)LEVELS.size();

// ---- the state ----------------------------------------------------------------------------------------------------------
struct Rect { double x, y, w, h; };
struct Spike { double x, y, w, h; int c, r; };
struct Exit { double x, y, w, h; int c, r; };
struct Candle { float x, y, ph; };
struct Lever { double x, y, w, h; char link; int time, timer; };
struct Plate { double x, y, w, h; char link; bool pressed; };
struct Door { double x, y, w, h; char link; double open; bool wasOpen; };
struct Lift { double x, w, h, y0, y1, y; char link; double speed, dy; };
struct Level {
    const LevelDef* def;
    char grid[ROWS][COLS];
    std::vector<Plate> plates; std::vector<Door> doors; std::vector<Lever> levers; std::vector<Lift> lifts;
    std::vector<Candle> candles; std::vector<Spike> spikes;
    double sx = 0, sy = 0;
    Exit exit{};
};
struct Rec { double x, y; int f, a; };
struct Ghost { std::vector<Rec> rec; bool on = false; double fade = 0, x = 0, y = 0, dx = 0, dy = 0; int face = 1, air = 0; };
struct Ground { int kind = 0, idx = 0; };              // 0 none, 1 a tile or ledge, 2 a lift, 3 a ghost
struct Player {
    double x, y, vx, vy; Ground ground; int face, coyote, buffer; bool prevJ, jumping, alive;
};
struct Particle { float x, y, vx, vy, life, max; ImU32 c; };
struct Fog { float x, y, r, v; };

std::string saveDir;
struct Best { int t, g; };
int unlocked = 0;
std::map<int, Best> best;
bool practice = false;

int level = 0;
Level L;
std::vector<Ghost> ghosts;
std::vector<Rec> run, rewindRun;
int t = 0;
Player player;
enum State { Splash, Play, Dead, Won, Rewind, End } state = Splash;
int stateT = 0;
bool paused = false;
double flare = 0;
std::vector<Particle> particles;
std::vector<Fog> fog;
std::string note;
int noteT = 0;
double planX = -1;
bool rewindSummon = false;

std::mt19937 mt{std::random_device{}()};
double mrand() { return std::uniform_real_distribution<double>(0.0, 1.0)(mt); }

void writeSave() {
    if (practice) return;
    std::ofstream f(saveDir + "save.txt");
    f << unlocked << "\n";
    for (const auto& kv : best) f << kv.first << " " << kv.second.t << " " << kv.second.g << "\n";
}
void readSave() {
    std::ifstream f(saveDir + "save.txt");
    if (!(f >> unlocked)) { unlocked = 0; return; }
    int lv, tt, g;
    while (f >> lv >> tt >> g) best[lv] = {tt, g};
}

// ---- the sound effects (site/sfx.js) ---------------------------------------------------------------------------------------
namespace Sfx {
using namespace Synth;
void tn(Wave w, float f0, float f1, double at, float len, float vol, float attack = 0.f, float vr = 0, float vd = 0) {
    tone(w, f0, f1, at, len, vol, attack > 0 ? attack : 0.005f, 0.0001f, vr, vd);
}
void jump() { tn(Square, 330, 520, now(), 0.09f, 0.06f); }
void land() { noise(now(), 0.05f, 0.12f, 600); }
void plate(bool on) { tn(Triangle, on ? 220.f : 180.f, on ? 330.f : 120.f, now(), 0.12f, 0.25f); }
void door() { const double t = now(); tn(Saw, 70, 55, t, 0.35f, 0.08f, 0.05f); noise(t, 0.3f, 0.06f, 400); }
void lever() { const double t = now(); tn(Square, 880, 660, t, 0.06f, 0.08f); tn(Square, 990, 990, t + 0.07, 0.06f, 0.06f); }
void die() { const double t = now(); tn(Square, 520, 60, t, 0.6f, 0.18f); noise(t, 0.4f, 0.25f, 1500); }
void rewind() { const double t = now(); tn(Saw, 1400, 90, t, 0.65f, 0.07f, 0.02f); tn(Sine, 700, 45, t, 0.65f, 0.18f, 0.02f); }
void summon() {                                            // a cold choir: detuned sines swelling up
    const double t = now();
    const float fs[] = {261.6f, 311.1f, 392.f, 523.3f};
    for (int i = 0; i < 4; i++) tn(Sine, fs[i], fs[i] * 1.003f, t + i * 0.06, 1.6f, 0.09f, 0.4f, 5.f + i, fs[i] * 0.01f);
}
void no() { const double t = now(); tn(Triangle, 160, 150, t, 0.25f, 0.25f); tn(Triangle, 120, 110, t + 0.22, 0.35f, 0.25f); }
void win() {
    const double t = now();
    const float fs[] = {523.3f, 659.3f, 784.f, 1046.5f, 1318.5f};
    for (int i = 0; i < 5; i++) { tn(Triangle, fs[i], fs[i], t + i * 0.09, 0.5f, 0.18f); tn(Sine, fs[i] * 2, fs[i] * 2, t + i * 0.09, 0.3f, 0.05f); }
}
}  // namespace Sfx

void burst(double x, double y, int n, double speed, std::vector<ImU32> cols) {
    if (particles.size() > 700) return;
    for (int i = 0; i < n; i++) {
        const double a = mrand() * PI * 2, v = speed * (0.2 + mrand() * 0.8), life = 30 + mrand() * 40;
        particles.push_back({(float)x, (float)y, (float)(std::cos(a) * v), (float)(std::sin(a) * v - 0.5), (float)life, (float)life,
                             cols[(size_t)(mrand() * cols.size())]});
    }
}

// ---- building a room ------------------------------------------------------------------------------------------------------
void parse(int idx) {
    const LevelDef& def = LEVELS[idx];
    L = Level();
    L.def = &def;
    int lv = 0;
    for (int r = 0; r < ROWS; r++) {
        const std::string row = r < (int)def.map.size() ? def.map[r] : FULL;
        for (int c = 0; c < COLS; c++) {
            const char ch = c < (int)row.size() ? row[c] : '#';
            char cell = '.';
            if (ch == '#' || ch == '-') cell = ch;
            else if (ch == '^') L.spikes.push_back({c * T + 4.0, r * T + 15.0, T - 8.0, 15.0, c, r});
            else if (ch == 'P') { L.sx = c * T + (T - PW) / 2; L.sy = r * T + T - PH; }
            else if (ch == 'E') L.exit = {c * T + 6.0, (double)r * T, T - 12.0, (double)T, c, r};
            else if (ch == 'i') L.candles.push_back({c * T + T / 2, r * T + T, c * 7.3f + r * 3.1f});
            else if (ch == '!') { const LeverDef& d = def.levers[lv++]; L.levers.push_back({(double)c * T, (double)r * T, T, T, d.link, d.time, 0}); }
            else if (ch >= 'a' && ch <= 'e') L.plates.push_back({c * T + 3.0, r * T + T - 6.0, T - 6.0, 6.0, ch, false});
            else if (ch >= 'A' && ch <= 'E') cell = ch;
            L.grid[r][c] = cell;
        }
    }
    for (int c = 0; c < COLS; c++)                         // doors: each vertical run of one letter is one door
        for (int r = 0; r < ROWS; r++) {
            const char g = L.grid[r][c];
            if (g >= 'A' && g <= 'E' && (r == 0 || L.grid[r - 1][c] != g)) {
                int n = 0;
                while (r + n < ROWS && L.grid[r + n][c] == g) n++;
                L.doors.push_back({(double)c * T, (double)r * T, T, (double)n * T, (char)(g - 'A' + 'a'), 0, false});
            }
        }
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) if (L.grid[r][c] >= 'A' && L.grid[r][c] <= 'E') L.grid[r][c] = '.';
    for (const LiftDef& l : def.lifts) L.lifts.push_back({(double)l.c * T, (double)l.w * T, 12, l.y, l.y2, l.y, l.link, 1.6, 0});
}

void startAttempt() {
    t = 0; run.clear();
    player = {L.sx, L.sy, 0, 0, {}, 1, 0, 0, false, false, true};
    for (Plate& p : L.plates) p.pressed = false;
    for (Lever& l : L.levers) l.timer = 0;
    for (Door& d : L.doors) { d.open = 0; d.wasOpen = false; }
    for (Lift& l : L.lifts) { l.y = l.y0; l.dy = 0; }
    for (Ghost& g : ghosts) { g.on = false; g.fade = 0; g.x = g.rec[0].x; g.y = g.rec[0].y; g.dx = g.dy = 0; }
}

void loadLevel(int i) {
    level = std::max(0, std::min(NLEVELS - 1, i));
    parse(level);
    ghosts.clear();
    startAttempt();
    state = Play; stateT = 0; paused = false;
    note = std::string(ROMAN[level]) + " \xC2\xB7 " + L.def->name;
    for (char& ch : note) ch = (char)std::toupper((unsigned char)ch);
    noteT = 150;
}

// ---- the simulation: one fixed step ---------------------------------------------------------------------------------------
bool overlap(const Rect& a, const Rect& b) { return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y; }
Rect box(double x, double y) { return {x, y, PW, PH}; }
bool active(char link) {
    bool any = false, all = true;
    for (const Plate& p : L.plates) if (p.link == link) { any = true; if (!p.pressed) all = false; }
    for (const Lever& l : L.levers) if (l.link == link) { any = true; if (l.timer <= 0) all = false; }
    return any && all;
}
Rect doorRect(const Door& d, double open) { return {d.x, d.y, d.w, d.h * (1 - open)}; }

std::vector<Rect> solidsAt(const Rect& rect) {
    std::vector<Rect> out;
    const int c0 = (int)std::floor(rect.x / T), c1 = (int)std::floor((rect.x + rect.w - 0.001) / T);
    const int r0 = (int)std::floor(rect.y / T), r1 = (int)std::floor((rect.y + rect.h - 0.001) / T);
    for (int r = r0; r <= r1; r++) for (int c = c0; c <= c1; c++) {
        if (r >= ROWS) continue;                           // below the map: the pit, nothing to stand on
        if (c < 0 || c >= COLS || r < 0 || L.grid[r][c] == '#') out.push_back({(double)c * T, (double)r * T, T, T});
    }
    for (const Door& d : L.doors) {
        const Rect dr = doorRect(d, d.open);
        if (dr.h > 0.5 && overlap(rect, dr)) out.push_back(dr);
    }
    return out;
}

struct OneWay { double x, y, w, dx, dy; int kind, idx; };
std::vector<OneWay> oneWays() {                          // things you can land on from above: lifts, and your ghosts' heads
    std::vector<OneWay> out;
    for (size_t i = 0; i < L.lifts.size(); i++) out.push_back({L.lifts[i].x, L.lifts[i].y, L.lifts[i].w, 0, L.lifts[i].dy, 2, (int)i});
    for (size_t i = 0; i < ghosts.size(); i++) if (ghosts[i].on) out.push_back({ghosts[i].x, ghosts[i].y, PW, ghosts[i].dx, ghosts[i].dy, 3, (int)i});
    return out;
}

void die() {
    player.alive = false;
    state = Dead; stateT = 0;
    burst(player.x + PW / 2, player.y + PH / 2, 60, 4, {C::pink, C::pink, C::white});
    Sfx::die();
}

void win() {
    state = Won; stateT = 0;
    burst(player.x + PW / 2, player.y + PH / 2, 90, 5, {C::pink, C::sky, C::white, C::ghost});
    Sfx::win();
    if (practice) return;
    unlocked = std::max(unlocked, level + 1);
    auto b = best.find(level);
    if (b == best.end() || t < b->second.t) best[level] = {t, (int)ghosts.size()};
    writeSave();
}

void physics(int bits) {
    Player& p = player;
    const bool Lk = bits & IN_L, Rk = bits & IN_R, J = (bits & IN_J) != 0;
    double carryDx = 0;
    if (p.ground.kind == 2) { p.y = L.lifts[p.ground.idx].y - PH; }   // standing on a lift or a ghost: it carries you
    else if (p.ground.kind == 3) {
        const Ghost& g = ghosts[p.ground.idx];
        if (g.on) { carryDx = g.dx; p.y = g.y - PH; }
    }
    const int dir = (Rk ? 1 : 0) - (Lk ? 1 : 0);
    if (dir) p.face = dir;
    const bool onGround = p.ground.kind != 0;
    const double acc = onGround ? ACC_G : ACC_A, target = dir * RUN;
    if (p.vx < target) p.vx = std::min(target, p.vx + acc); else if (p.vx > target) p.vx = std::max(target, p.vx - acc);

    if (J && !p.prevJ) p.buffer = BUFFER; else if (p.buffer > 0) p.buffer--;
    p.prevJ = J;
    if (p.buffer > 0 && p.coyote > 0) {
        p.vy = JUMP; p.buffer = 0; p.coyote = 0; p.jumping = true; p.ground = {};
        Sfx::jump();
    }
    if (p.jumping && !J && p.vy < CUT) p.vy = CUT;
    if (p.vy >= 0) p.jumping = false;
    p.vy = std::min(MAXFALL, p.vy + GRAV);

    const double dx = p.vx + carryDx;                     // across, then down, each against the walls
    p.x += dx;
    for (const Rect& r : solidsAt(box(p.x, p.y))) {
        if (dx > 0 || (dx == 0 && p.x + PW / 2 < r.x + r.w / 2)) p.x = std::min(p.x, r.x - PW);
        else p.x = std::max(p.x, r.x + r.w);
        p.vx = 0;
    }
    const double prevBottom = p.y + PH;
    const bool wasGround = p.ground.kind != 0;
    p.y += p.vy;
    p.ground = {};
    for (const Rect& r : solidsAt(box(p.x, p.y))) {
        if (p.vy >= 0) { p.y = std::min(p.y, r.y - PH); p.ground = {1, 0}; }
        else p.y = std::max(p.y, r.y + r.h);
        p.vy = 0;
    }
    if (p.vy >= 0 && p.ground.kind == 0) {
        const double bottom = p.y + PH;
        bool has = false; double bestY = 0; Ground bg;
        const int c0 = (int)std::floor(p.x / T), c1 = (int)std::floor((p.x + PW - 0.001) / T);
        for (int r = (int)std::floor(prevBottom / T) - 1; r <= (int)std::floor(bottom / T); r++) {
            if (r < 0 || r >= ROWS) continue;
            for (int c = c0; c <= c1; c++) if (c >= 0 && c < COLS && L.grid[r][c] == '-') {
                const double top = r * T;
                if (prevBottom <= top + 0.5 && bottom >= top && (!has || top < bestY)) { has = true; bestY = top; bg = {1, 0}; }
            }
        }
        for (const OneWay& o : oneWays()) {
            const double tol = 0.5 + std::max(0.0, -o.dy) + std::fabs(o.dy);
            if (p.x < o.x + o.w && p.x + PW > o.x && prevBottom <= o.y + tol && bottom >= o.y && (!has || o.y < bestY)) {
                has = true; bestY = o.y; bg = {o.kind, o.idx};
            }
        }
        if (has) { p.y = bestY - PH; p.vy = 0; p.ground = bg; }
    }
    if (p.ground.kind != 0 && !wasGround) { Sfx::land(); burst(p.x + PW / 2, p.y + PH, 5, 1, {C::pink, C::white}); }
    p.coyote = p.ground.kind != 0 ? COYOTE : p.coyote - 1;
}

void simFrame(int bits) {
    for (Ghost& g : ghosts) {                            // 1. the ghosts move to where they were at this moment
        const bool was = g.on;
        g.on = t < (int)g.rec.size();
        if (g.on) {
            const Rec& s = g.rec[t];
            const double px = g.x, py = g.y;
            g.x = s.x; g.y = s.y; g.face = s.f; g.air = s.a;
            g.dx = t > 0 ? g.x - px : 0; g.dy = t > 0 ? g.y - py : 0;
        } else {
            g.dx = g.dy = 0;
            if (was) { g.fade = 1; burst(g.x + PW / 2, g.y + PH / 2, 24, 2, {C::sky, C::ghost}); }
        }
    }
    std::vector<Rect> bodies;
    for (const Ghost& g : ghosts) if (g.on) bodies.push_back(box(g.x, g.y));
    if (player.alive) bodies.push_back(box(player.x, player.y));

    for (Plate& p : L.plates) {                          // 2. plates, levers, doors and lifts
        const Rect hit{p.x, p.y - 3, p.w, p.h + 3};
        bool on = false;
        for (const Rect& b : bodies) if (overlap(b, hit)) on = true;
        if (on != p.pressed) { Sfx::plate(on); if (on) burst(p.x + p.w / 2, p.y, 8, 1.5, {C::green}); }
        p.pressed = on;
    }
    for (Lever& l : L.levers) {
        bool on = false;
        for (const Rect& b : bodies) if (overlap(b, {l.x, l.y, l.w, l.h})) on = true;
        if (on && l.timer <= 0) Sfx::lever();
        if (on) l.timer = l.time; else if (l.timer > 0) l.timer--;
    }
    for (Door& d : L.doors) {
        if (active(d.link)) d.open = std::min(1.0, d.open + 0.1);
        else {
            const double n = std::max(0.0, d.open - 0.05);
            if (!(player.alive && overlap(box(player.x, player.y), doorRect(d, n)))) d.open = n;   // never shuts on the living
        }
        const bool isOpen = d.open > 0.5;
        if (isOpen != d.wasOpen) Sfx::door();
        d.wasOpen = isOpen;
    }
    for (Lift& l : L.lifts) {
        const double target = active(l.link) ? l.y1 : l.y0;
        l.dy = std::max(-l.speed, std::min(l.speed, target - l.y));
        l.y += l.dy;
    }

    if (player.alive) physics(bits);                      // 3. the living
    run.push_back({player.x, player.y, player.face, player.ground.kind ? 0 : 1});
    t++;

    if (player.alive) {                                   // 4. hazards and the way out
        const Rect pb = box(player.x, player.y);
        bool dead = player.y > H + 40;
        for (const Spike& s : L.spikes) if (overlap(pb, {s.x, s.y, s.w, s.h})) dead = true;
        if (dead) die();
        else if (overlap(pb, {L.exit.x, L.exit.y, L.exit.w, L.exit.h})) win();
    }
}

void beginRewind(bool summon) {
    if (state != Play && state != Dead) return;
    if (summon && (int)ghosts.size() >= L.def->ghosts) {
        summon = false; note = "THE BOARD SAYS NO \xC2\xB7 no spirits left"; noteT = 120;
        Sfx::no();
    }
    if (summon && run.size() < 20) summon = false;        // a blink of a run is not worth a spirit
    rewindSummon = summon; rewindRun = run;
    state = Rewind; stateT = 0;
    Sfx::rewind();
}

void finishRewind() {
    if (rewindSummon) {
        Ghost g; g.rec = rewindRun;
        ghosts.push_back(g);
        flare = 1;
        Sfx::summon();
    }
    startAttempt();
    if (rewindSummon) burst(player.x + PW / 2, player.y + PH / 2, 50, 3, {C::sky, C::ghost, C::white});
    state = Play; stateT = 0;
}

void banish() {
    if (state != Play && state != Dead) return;
    ghosts.clear();
    note = "THE SPIRITS ARE BANISHED"; noteT = 90;
    beginRewind(false);
}

void tick() {                                             // the things that fade and fly, one fixed step
    if (flare > 0) flare = std::max(0.0, flare - 0.008);
    if (noteT > 0) noteT--;
    for (Ghost& g : ghosts) if (g.fade > 0) g.fade = std::max(0.0, g.fade - 0.03);
    for (int i = (int)particles.size() - 1; i >= 0; i--) {
        Particle& p = particles[i];
        p.x += p.vx; p.y += p.vy; p.vx *= 0.96f; p.vy = p.vy * 0.96f + 0.05f;
        if (--p.life <= 0) particles.erase(particles.begin() + i);
    }
}

// ---- drawing ----------------------------------------------------------------------------------------------------------
Canvas* cx = nullptr;
#define ctx (*cx)
void glow(ImU32 col, float b) { ctx.shadowColor = col; ctx.shadowBlur = b; ctx.fillStyle = col; ctx.strokeStyle = col; }
void noGlow() { ctx.shadowBlur = 0; }
bool isWall(int r, int c) { return r < 0 || c < 0 || c >= COLS || (r < ROWS && L.grid[r][c] == '#'); }

void drawStatic() {
    ctx.fillStyle = C::black; ctx.fillRect(0, 0, W, H);
    // tall arched windows far behind, with a few cold stars in them
    long long n = level * 97 + 13;
    auto rnd = [&n] { n = (n * 9301 + 49297) % 233280; return (float)n / 233280.f; };
    for (int i = 0; i < 5; i++) {
        const float wx = 60 + i * 190 + rnd() * 40, wy = 70 + rnd() * 50, ww = 54, wh = 170;
        ctx.fillStyle = rgb("#000c16");
        ctx.beginPath(); ctx.moveTo(wx, wy + wh); ctx.lineTo(wx, wy + ww / 2); ctx.arc(wx + ww / 2, wy + ww / 2, ww / 2, PI, 0); ctx.lineTo(wx + ww, wy + wh); ctx.closePath(); ctx.fill();
        ctx.strokeStyle = C::navy; ctx.lineWidth = 3; ctx.stroke();
        ctx.fillStyle = C::deep;
        for (int k = 0; k < 6; k++) { const float a = rnd(), b = rnd(); ctx.fillRect(wx + 6 + a * (ww - 12), wy + 10 + b * (wh - 20), 1.5f, 1.5f); }
        ctx.fillStyle = C::navy; ctx.fillRect(wx + ww / 2 - 1, wy + 6, 2, wh - 6); ctx.fillRect(wx, wy + wh * 0.55f, ww, 2);
    }
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) {
        const char g = L.grid[r][c];
        const float x = c * T, y = r * T;
        if (g == '#') {
            ctx.fillStyle = C::navy; ctx.fillRect(x, y, T, T);
            ctx.fillStyle = rgb("#00213a");                // stone seams
            const int off = (r % 2) * 15;
            ctx.fillRect(x, y + 14, T, 1);
            ctx.fillRect(x + (float)((off + 7) % (int)T), y, 1, 14);
            ctx.fillRect(x + (float)((off + 22) % (int)T), y + 15, 1, 15);
            if (!isWall(r - 1, c)) { ctx.fillStyle = C::blue; ctx.fillRect(x, y, T, 2); }   // edges that face open air
            ctx.fillStyle = C::deep;
            if (!isWall(r + 1, c) && r + 1 < ROWS) ctx.fillRect(x, y + T - 1, T, 1);
            if (!isWall(r, c - 1)) ctx.fillRect(x, y, 1, T);
            if (!isWall(r, c + 1)) ctx.fillRect(x + T - 1, y, 1, T);
        } else if (g == '-') {
            ctx.fillStyle = C::deep; ctx.fillRect(x, y, T, 7);
            ctx.fillStyle = C::sky; ctx.fillRect(x, y, T, 2);
            ctx.fillStyle = C::navy; ctx.fillRect(x + 4, y + 7, 2, 4); ctx.fillRect(x + T - 6, y + 7, 2, 4);
        }
    }
    ctx.shadowColor = C::sky; ctx.shadowBlur = 8;
    for (const Spike& sp : L.spikes) {
        const float x = sp.c * T, y = sp.r * T;
        ctx.fillStyle = rgb("#bfe3f5");
        for (int k = 0; k < 3; k++) {
            ctx.beginPath(); ctx.moveTo(x + k * 10, y + T); ctx.lineTo(x + k * 10 + 5, y + 12); ctx.lineTo(x + k * 10 + 10, y + T); ctx.closePath(); ctx.fill();
        }
    }
    ctx.shadowBlur = 0;
}

void drawFigure(double xd, double yd, int face, bool ghost, float alpha, double now) {
    const float x = (float)xd, y = (float)yd, pw = (float)PW, ph = (float)PH;
    ctx.save();
    ctx.globalAlpha = alpha;
    if (ghost) {
        glow(C::sky, 18);
        ctx.fillStyle = C::ghost;
        ctx.beginPath();                                   // a sheet with a wavy hem
        ctx.moveTo(x, y + ph);
        ctx.lineTo(x, y + 8);
        ctx.arc(x + pw / 2, y + 9, pw / 2, PI, 0);
        ctx.lineTo(x + pw, y + ph);
        for (int k = 3; k >= 0; k--) ctx.lineTo(x + k * pw / 4 + pw / 8, y + ph - 4 + 3 * (float)std::sin(now / 120 + k * 2));
        ctx.closePath(); ctx.fill();
        noGlow();
        ctx.fillStyle = C::deep;
        ctx.fillRect(x + pw / 2 + face * 3 - 3, y + 8, 3, 4); ctx.fillRect(x + pw / 2 + face * 3 + 2, y + 8, 3, 4);
    } else {
        glow(C::pink, 14);
        ctx.beginPath(); ctx.arc(x + pw / 2, y + 7, 7, 0, PI * 2); ctx.fill();         // head
        ctx.fillRect(x + 2, y + 12, pw - 4, ph - 12);                                     // body
        noGlow();
        ctx.fillStyle = C::black;
        ctx.fillRect(x + pw / 2 + face * 2 - 2, y + 5, 2, 3); ctx.fillRect(x + pw / 2 + face * 2 + 2, y + 5, 2, 3);
    }
    ctx.restore();
}

void drawCandle(const Candle& cd, double now) {
    const float fl = (float)(std::sin(now / 90 + cd.ph) * 0.5 + std::sin(now / 37 + cd.ph * 2) * 0.3);
    const float big = 1 + (float)flare * 2.2f, x = cd.x, y = cd.y;
    ctx.fillStyle = rgb("#d9d4e8"); ctx.fillRect(x - 3, y - 14, 6, 14);                // the wax
    ctx.fillStyle = C::navy; ctx.fillRect(x - 6, y - 2, 12, 2);
    const ImU32 col = flare > 0.05 ? C::sky : C::pink;
    glow(col, 20 + 25 * (float)flare);
    ctx.beginPath();
    ctx.ellipse(x + fl, y - 19 - 3 * big, 3 * big, (6 + fl) * big, 0, 0, PI * 2);
    ctx.fill();
    ctx.fillStyle = C::white; noGlow();
    ctx.beginPath(); ctx.ellipse(x + fl * 0.5f, y - 18 - 2 * big, 1.2f * big, 3 * big, 0, 0, PI * 2); ctx.fill();
    ctx.globalAlpha = 0.07f + 0.05f * fl + 0.18f * (float)flare;                         // a pool of light
    ctx.fillStyle = col;
    ctx.beginPath(); ctx.arc(x, y - 20, 40 + 50 * (float)flare, 0, PI * 2); ctx.fill();
    ctx.globalAlpha = 1;
}

std::string up(char c) { return std::string(1, (char)std::toupper((unsigned char)c)); }

void drawWorld(double now, bool hasGF, int ghostFrame) {
    drawStatic();
    ctx.fillStyle = C::navy;                             // fog drifting through
    for (Fog& f : fog) {
        f.x += f.v; if (f.x > W + 120) f.x = -120;
        ctx.globalAlpha = 0.18f; ctx.beginPath(); ctx.arc(f.x, f.y, f.r, 0, PI * 2); ctx.fill();
    }
    ctx.globalAlpha = 1;
    for (const Candle& cd : L.candles) drawCandle(cd, now);

    const float ex = L.exit.c * T + T / 2, ey = L.exit.r * T + T;   // the way out: an arch of pink light
    glow(C::pink, 22 + 8 * (float)std::sin(now / 300));
    ctx.lineWidth = 3;
    ctx.beginPath(); ctx.moveTo(ex - 13, ey); ctx.lineTo(ex - 13, ey - 34); ctx.arc(ex, ey - 34, 13, PI, 0); ctx.lineTo(ex + 13, ey); ctx.stroke();
    noGlow();
    ctx.fillStyle = rgb("#2a0a2a"); ctx.globalAlpha = 0.7f;
    ctx.beginPath(); ctx.moveTo(ex - 11, ey); ctx.lineTo(ex - 11, ey - 34); ctx.arc(ex, ey - 34, 11, PI, 0); ctx.lineTo(ex + 11, ey); ctx.fill();
    ctx.globalAlpha = 1;
    for (int m = 0; m < 4; m++) {
        const double a = now / 600 + m * 1.57;
        ctx.fillStyle = C::pink; ctx.fillRect(ex + (float)std::cos(a) * 6 - 1, ey - 22 + (float)std::sin(a * 1.3) * 14, 2, 2);
    }

    for (const Plate& p : L.plates) {
        const float x = (float)p.x, y = (float)p.y, w = (float)p.w, h = (float)p.h;
        if (p.pressed) { glow(C::green, 16); ctx.fillRect(x, y + 3, w, 3); }
        else { noGlow(); ctx.fillStyle = C::deep; ctx.fillRect(x, y, w, h); ctx.fillStyle = C::green; ctx.globalAlpha = 0.5f; ctx.fillRect(x, y, w, 1); ctx.globalAlpha = 1; }
        noGlow();
        ctx.fillStyle = p.pressed ? C::green : C::blue; ctx.font(11); ctx.textAlign = 1;
        ctx.fillText(up(p.link), x + w / 2, y - 8);
    }
    for (const Lever& l : L.levers) {
        const bool on = l.timer > 0;
        const float bx = (float)l.x + T / 2, by = (float)l.y + T;
        ctx.fillStyle = C::deep; ctx.fillRect(bx - 8, by - 5, 16, 5);
        ctx.strokeStyle = on ? C::green : C::sky; ctx.lineWidth = 3;
        if (on) glow(C::green, 12);
        ctx.beginPath(); ctx.moveTo(bx, by - 4); ctx.lineTo(bx + (on ? 9 : -9), by - 20); ctx.stroke();
        ctx.beginPath(); ctx.arc(bx + (on ? 9 : -9), by - 21, 3.5f, 0, PI * 2); ctx.fillStyle = on ? C::green : C::sky; ctx.fill();
        noGlow();
        if (on) {
            ctx.strokeStyle = C::green; ctx.lineWidth = 2;
            ctx.beginPath(); ctx.arc(bx, by - 14, 15, -PI / 2, -PI / 2 + PI * 2 * l.timer / l.time); ctx.stroke();
        }
        ctx.fillStyle = on ? C::green : C::blue; ctx.font(11); ctx.textAlign = 1;
        ctx.fillText(up(l.link), bx, (float)l.y - 2);
    }
    for (const Door& d : L.doors) {
        const Rect r = doorRect(d, d.open);
        const bool lit = active(d.link);
        if (r.h > 0.5) {
            const float x = (float)r.x, y = (float)r.y, w = (float)r.w, h = (float)r.h;
            ctx.fillStyle = C::navy; ctx.fillRect(x + 2, y, w - 4, h);
            ctx.fillStyle = lit ? C::green : C::deep;
            for (int k = 0; k < 4; k++) ctx.fillRect(x + 4 + k * 7, y, 2, h);
            glow(lit ? C::green : C::sky, 10);
            ctx.fillRect(x + 2, y + h - 3, w - 4, 3);
            noGlow();
        }
        ctx.fillStyle = lit ? C::green : C::sky; ctx.font(12, true); ctx.textAlign = 1;
        ctx.fillText(up(d.link), (float)d.x + T / 2, (float)(d.y + d.h - 6));
    }
    for (const Lift& l : L.lifts) {
        const float x = (float)l.x, w = (float)l.w, y = (float)l.y, y1 = (float)l.y1;
        ctx.strokeStyle = C::deep; ctx.lineWidth = 1; ctx.setLineDash(3, 4);
        ctx.beginPath(); ctx.moveTo(x + 6, y1 - 30); ctx.lineTo(x + 6, y); ctx.moveTo(x + w - 6, y1 - 30); ctx.lineTo(x + w - 6, y); ctx.stroke();
        ctx.setLineDash(0, 0);
        const bool lit = active(l.link);
        ctx.fillStyle = C::deep; ctx.fillRect(x, y, w, (float)l.h);
        glow(lit ? C::green : C::sky, 12); ctx.fillRect(x, y, w, 2); noGlow();
        ctx.fillStyle = lit ? C::green : C::sky; ctx.font(10); ctx.textAlign = 1;
        ctx.fillText(up(l.link), x + w / 2, y + 11);
    }

    for (size_t i = 0; i < ghosts.size(); i++) {         // the dead, with a faint trail behind
        const Ghost& g = ghosts[i];
        int f = hasGF ? ghostFrame : t - 1;
        if (f < 0) f = 0;
        if (f < (int)g.rec.size()) {
            for (int k = 3; k >= 1; k--) {
                const Rec& s = g.rec[std::max(0, f - k * 5)];
                drawFigure(s.x, s.y, s.f, true, 0.08f * (4 - k), now);
            }
            const Rec& cur = g.rec[f];
            drawFigure(cur.x, cur.y, cur.f, true, 0.62f + 0.08f * (float)std::sin(now / 200 + i), now);
            ctx.fillStyle = C::sky; ctx.font(10); ctx.textAlign = 1;
            ctx.fillText(ROMAN[i], (float)(cur.x + PW / 2), (float)cur.y - 5);
        } else if (g.fade > 0 && !hasGF) {
            const Rec& last = g.rec.back();
            drawFigure(last.x, last.y - (1 - g.fade) * 20, last.f, true, (float)g.fade * 0.6f, now);
        }
    }
}

void roundRect(float x, float y, float w, float h, float r) {
    ctx.beginPath(); ctx.moveTo(x + r, y); ctx.lineTo(x + w - r, y); ctx.arc(x + w - r, y + r, r, -PI / 2, 0);
    ctx.lineTo(x + w, y + h - r); ctx.arc(x + w - r, y + h - r, r, 0, PI / 2);
    ctx.lineTo(x + r, y + h); ctx.arc(x + r, y + h - r, r, PI / 2, PI);
    ctx.lineTo(x, y + r); ctx.arc(x + r, y + r, r, PI, PI * 1.5f); ctx.closePath();
}

void drawOuija() {
    const int max = L.def->ghosts, left = max - (int)ghosts.size();
    const float bw = 250, bh = 66, bx = W - bw - 12, by = 10;
    ctx.fillStyle = C::navy; ctx.strokeStyle = C::deep; ctx.lineWidth = 2;
    roundRect(bx, by, bw, bh, 12); ctx.fill(); ctx.stroke();
    ctx.font(11); ctx.textAlign = 1; ctx.fillStyle = C::sky;
    ctx.fillText("YES", bx + 24, by + 18); ctx.fillText("NO", bx + bw - 22, by + 18);
    ctx.fillText("\xE2\x98\xBE", bx + 24, by + 34); ctx.fillText("\xE2\x98\xBC", bx + bw - 22, by + 34);
    ctx.fillStyle = C::blue; ctx.font(9); ctx.fillText("SPIRITS LEFT", bx + bw / 2, by + 14);
    std::vector<ImVec2> xs;
    for (int i = 0; i <= max; i++) {
        const float x = bx + 60 + (max ? i * (bw - 120) / max : (bw - 120) / 2);
        const float y = by + 44 - std::sin(PI * (max ? (float)i / max : 0.5f)) * 12;
        xs.push_back({x, y});
        ctx.fillStyle = i == left ? C::pink : C::sky; ctx.font(18);
        ctx.fillText(std::to_string(i), x, y + 6);
    }
    const ImVec2 target = xs[std::max(0, std::min(max, left))];
    if (planX < 0) planX = target.x;
    planX += (target.x - planX) * 0.12;
    const float px = (float)planX, py = target.y + 3;
    // the planchette: a heart-shaped pointer with a window over the number
    glow(C::pink, 12); ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(px, py - 22);
    ctx.bezierTo(px + 22, py - 18, px + 20, py + 12, px, py + 16);
    ctx.bezierTo(px - 20, py + 12, px - 22, py - 18, px, py - 22);
    ctx.stroke(); noGlow();
    ctx.beginPath(); ctx.arc(px, py - 3, 10, 0, PI * 2); ctx.strokeStyle = C::pink; ctx.lineWidth = 1.5f; ctx.stroke();
}

std::string fmt(int frames) {
    const double s = frames / 60.0;
    const int m = (int)std::floor(s / 60);
    char b[32];
    std::snprintf(b, sizeof b, "0%.1f", s - m * 60);
    std::string r = b;
    return std::to_string(m) + ":" + r.substr(r.size() - 4);
}

void centreText(const std::string& text, float y, float size, ImU32 col) {
    ctx.textAlign = 1; ctx.font(size);
    ctx.fillStyle = col; ctx.fillText(text, W / 2, y);
}

}  // namespace

// ---- the outside ---------------------------------------------------------------------------------------------------------
void Game::init(const char* folder) {
    saveDir = folder;
    readSave();
    Synth::setMuted([] { std::ifstream f(saveDir + "muted.txt"); int m = 0; f >> m; return m == 1; }());
    for (int f = 0; f < 9; f++) fog.push_back({(float)(mrand() * W), (float)(120 + mrand() * 380), (float)(60 + mrand() * 90), (float)(0.1 + mrand() * 0.25)});
}

void Game::start() {
    practice = false;
    int first = 0;
    while (first < NLEVELS - 1 && best.count(first)) first++;
    loadLevel(first);
}

void Game::load(int room) { practice = true; loadLevel(room); }

bool Game::anyKey() {
    if (state == Won && stateT > 45) {
        if (level + 1 < NLEVELS) loadLevel(level + 1);
        else { state = End; stateT = 0; }
        return true;
    }
    if (state == End && stateT > 60) { loadLevel(0); return true; }
    return false;
}

bool Game::command(char k) {
    if (k == 'p') { if (state == Play || state == Dead) paused = !paused; return true; }
    if (k == 'm') {
        Synth::setMuted(!Synth::muted());
        { std::ofstream f(saveDir + "muted.txt"); f << (Synth::muted() ? 1 : 0); }
        note = Synth::muted() ? "SOUND OFF" : "SOUND ON"; noteT = 60; return true;
    }
    if (paused) return false;
    if (k == 'r') { beginRewind(true); return true; }
    if (k == 't') { beginRewind(false); return true; }
    if (k == 'x') { banish(); return true; }
    if (k == '[') { if (level > 0) loadLevel(level - 1); return true; }
    if (k == ']') {
        if ((level < unlocked || practice) && level + 1 < NLEVELS) loadLevel(level + 1);
        else { note = "SOLVE THIS ONE FIRST"; noteT = 70; }
        return true;
    }
    return false;
}

void Game::step(int bits) {
    if (paused) return;
    if (state == Play) simFrame(bits);
    else if (state == Rewind) { stateT++; if (stateT >= 40) finishRewind(); }
    else if (state != Splash) stateT++;
    tick();
}

void Game::draw(Canvas& c, double now) {
    cx = &c;
    const bool rewinding = state == Rewind;
    const float k = rewinding ? stateT / 40.f : 0.f;
    const int rf = rewinding ? (int)std::floor((rewindRun.size() - 1.0) * (1 - k)) : 0;
    drawWorld(now, rewinding, rf);

    if (rewinding) {
        if (!rewindRun.empty()) { const Rec& s = rewindRun[std::max(0, rf)]; drawFigure(s.x, s.y, s.f, false, 0.8f, now); }
    } else if (player.alive) {
        drawFigure(player.x, player.y, player.face, false, 1, now);
    }

    for (const Particle& p : particles) {
        ctx.globalAlpha = p.life / p.max;
        glow(p.c, 8);
        ctx.fillRect(p.x - 1.5f, p.y - 1.5f, 3, 3);
    }
    ctx.globalAlpha = 1; noGlow();

    if (rewinding) {                                     // the tape running back
        ctx.fillStyle = C::sky; ctx.globalAlpha = 0.12f;
        for (float y = (float)std::fmod(now / 4, 12.0); y < H; y += 12) ctx.fillRect(0, y, W, 2);
        ctx.globalAlpha = 0.25f; ctx.fillRect(0, (1 - k) * H, W, 6);
        ctx.globalAlpha = 1;
        centreText(rewindSummon ? "\xE2\x97\x80\xE2\x97\x80  SUMMONING" : "\xE2\x97\x80\xE2\x97\x80  REWIND", H / 2, 40, rewindSummon ? C::sky : C::pink);
    }

    // HUD
    std::string title = std::string(ROMAN[level]) + " \xC2\xB7 " + L.def->name;
    for (char& ch : title) ch = (char)std::toupper((unsigned char)ch);
    ctx.textAlign = 0; ctx.font(18);
    ctx.fillStyle = C::pink; ctx.fillText(title, 14, 28);
    ctx.font(14); ctx.fillStyle = C::sky;
    auto b = best.find(level);
    ctx.fillText("Time " + fmt(t) + (b != best.end() ? "    Best " + fmt(b->second.t) : "") + (Synth::muted() ? "    \xE2\x99\xAA off" : ""), 14, 50);
    drawOuija();
    ctx.textAlign = 1; ctx.font(14, false, true); ctx.fillStyle = rgb("#c8e8f8");
    ctx.globalAlpha = 0.85f;
    ctx.fillText(L.def->hint, W / 2, H - 12);
    ctx.globalAlpha = 0.6f; ctx.font(11); ctx.textAlign = 0; ctx.fillStyle = C::sky;
    ctx.fillText("\xE2\x86\x90\xE2\x86\x92 move  \xE2\x86\x91/Space jump  R summon  T retry  X banish  [ ] level  P pause  M mute", 14, H - 30);
    ctx.globalAlpha = 1;

    if (noteT > 0) { ctx.globalAlpha = std::min(1.f, noteT / 30.f); centreText(note, 110, 26, C::pink); ctx.globalAlpha = 1; }

    auto dim = [] { ctx.fillStyle = C::black; ctx.globalAlpha = 0.6f; ctx.fillRect(0, 0, W, H); ctx.globalAlpha = 1; };
    if (paused) { dim(); centreText("PAUSED", H / 2 - 10, 56, C::pink); centreText("P to carry on", H / 2 + 30, 18, C::white); }
    else if (state == Dead && stateT > 20) {
        centreText("YOU DIED", H / 2 - 20, 48, C::pink);
        centreText((int)ghosts.size() < L.def->ghosts ? "R  summon this run as a ghost     T  try again" : "T  try again     X  banish your ghosts", H / 2 + 18, 18, C::white);
    } else if (state == Won && stateT > 30) {
        dim();
        centreText("THE SPIRITS ARE AT REST", H / 2 - 30, 40, C::pink);
        centreText("Solved in " + fmt(t) + " with " + std::to_string(ghosts.size()) + (ghosts.size() == 1 ? " ghost" : " ghosts"), H / 2 + 10, 20, C::sky);
        centreText(level + 1 < NLEVELS ? "Any key for the next room" : "Any key", H / 2 + 44, 16, C::white);
    } else if (state == End) {
        dim();
        int total = 0;
        for (int i = 0; i < NLEVELS; i++) if (best.count(i)) total += best[i].t;
        centreText("THE S\xC3\x89" "ANCE IS OVER", H / 2 - 30, 46, C::pink);
        centreText("Every room solved. Best total " + fmt(total), H / 2 + 12, 20, C::sky);
        centreText("Any key to begin again", H / 2 + 46, 16, C::white);
    }
}
