// main.cpp - SÉANCE: the window, the splash (the title, the Cursed logo, the copyright), the game loop.
//
//   any key / click   start (on the splash)       Left Right / A D   move
//   Up / W / Space / Z   jump (hold for higher)    R   rewind and summon this run as a ghost
//   T   rewind and try again, no ghost             X   banish all ghosts and restart the room
//   [ ] / PgUp PgDn   previous / next room          P   pause        M   mute
//   F11 fullscreen       Esc   back to the splash (from the splash: quit)
#include "canvas.h"
#include "game.h"
#include "synth.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

std::string base;                                   // the folder the exe is in
std::string asset(const char* name) { return base + "assets/" + name; }

struct Texture { GLuint id = 0; int w = 0, h = 0; };

Texture loadTexture(const std::string& path) {
    Texture t;
    int n = 0;
    unsigned char* px = stbi_load(path.c_str(), &t.w, &t.h, &n, 4);
    if (!px) return t;
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);
    return t;
}

// UTF-8 text split into characters (for letter spacing)
std::vector<std::string> chars(const std::string& s) {
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size();) {
        size_t n = 1;
        const unsigned char c = (unsigned char)s[i];
        if (c >= 0xF0) n = 4; else if (c >= 0xE0) n = 3; else if (c >= 0xC0) n = 2;
        out.push_back(s.substr(i, n));
        i += n;
    }
    return out;
}

// text with letter spacing, centred on cx
void spaced(Canvas& c, const std::string& s, float cx, float y, float spacing) {
    const std::vector<std::string> cs = chars(s);
    float w = -spacing;
    for (const std::string& g : cs) w += c.measureText(g) + spacing;
    float x = cx - w * 0.5f;
    const int align = c.textAlign;
    c.textAlign = 0;
    for (const std::string& g : cs) { c.fillText(g, x, y); x += c.measureText(g) + spacing; }
    c.textAlign = align;
}

void splash(Canvas& c, const Texture& logo, double t) {
    const ImU32 pink = rgb("#f088e9"), sky = rgb("#34a9e3"), grey = rgb("#8fa3b3");
    c.fillStyle = rgb("#020611"); c.fillRect(0, 0, c.W, c.H);
    const float cx = c.W / 2;
    c.font(84); c.fillStyle = pink;
    spaced(c, "S\xC3\x89" "ANCE", cx, 178, 84 * 0.18f);
    c.font(15); c.fillStyle = sky;
    spaced(c, "AN ARCADE PHANTASM GAME", cx, 222, 15 * 0.25f);
    c.font(13, false, true); c.fillStyle = rgb("#c8e8f8"); c.textAlign = 1;
    c.fillText("Run. Rewind. Your past selves rise as ghosts and repeat every move you made.", cx, 252);
    if (logo.id) {
        const float lw = 200, lh = lw * logo.h / logo.w;
        c.image((ImTextureID)(intptr_t)logo.id, cx - lw / 2, 272, lw, lh);
    }
    c.font(12); c.fillStyle = grey;
    c.fillText("\xC2\xA9 Cursed Entertainment 2026", cx, 398);
    if (std::fmod(t, 1.2) < 0.6) { c.font(16); c.fillStyle = pink; spaced(c, "Press any key or click to start", cx, 438, 16 * 0.08f); }
    c.font(11); c.fillStyle = rgb("#5d7385"); c.textAlign = 1;
    c.fillText("\xE2\x86\x90 \xE2\x86\x92 / A D move  \xC2\xB7  \xE2\x86\x91 / W / Space jump  \xC2\xB7  R summon  \xC2\xB7  T retry  \xC2\xB7  "
               "X banish  \xC2\xB7  [ ] room  \xC2\xB7  P pause  \xC2\xB7  M mute  \xC2\xB7  F11 fullscreen  \xC2\xB7  Esc quit", cx, c.H - 22);
}

}  // namespace

int main(int argc, char** argv) {
    char* bp = SDL_GetBasePath();
    base = bp ? bp : "";
    SDL_free(bp);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_Window* win = SDL_CreateWindow("S\xC3\x89" "ANCE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) return 1;
    SDL_GLContext gl = SDL_GL_CreateContext(win);
    SDL_GL_MakeCurrent(win, gl);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    // Georgia, as the browser game; the arrows, moons, suns and notes it lacks come from Segoe UI Symbol
    static const ImWchar symbols[] = {0x2190, 0x2193, 0x25C0, 0x25C0, 0x2600, 0x26FF, 0};
    Canvas::loadFonts("C:/Windows/Fonts/georgia.ttf", "C:/Windows/Fonts/georgiab.ttf", "C:/Windows/Fonts/georgiai.ttf",
                      "C:/Windows/Fonts/seguisym.ttf", symbols);
    ImGui_ImplSDL2_InitForOpenGL(win, gl);
    ImGui_ImplOpenGL3_Init("#version 330");

    const Texture cursed = loadTexture(asset("cursed-logo.png"));
    if (SDL_Surface* icon = SDL_LoadBMP(asset("icon.bmp").c_str())) { SDL_SetWindowIcon(win, icon); SDL_FreeSurface(icon); }
    Synth::open(0.32f);

    // the solved rooms, best times and the mute choice, kept in the user's settings folder
    char* pp = SDL_GetPrefPath("Cursed Entertainment", "SEANCE");
    Game::init(pp ? pp : "");
    SDL_free(pp);

    bool started = false, fullscreen = false, quit = false;
    std::string capture;                            // --capture file.bmp seconds: save one frame of the window, then quit
    double captureAt = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--capture" && i + 2 < argc) { capture = argv[i + 1]; captureAt = std::atof(argv[i + 2]); i += 2; }
        else if (a == "--room" && i + 1 < argc) { started = true; Game::load(std::atoi(argv[++i]) - 1); }
    }
    bool held[SDL_NUM_SCANCODES] = {};             // the keys held down, from the key events (losing focus lets go)
    Canvas canvas;
    Uint64 last = SDL_GetPerformanceCounter();
    double acc = 0;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) std::fill(std::begin(held), std::end(held), false);
            if (e.type == SDL_KEYUP) held[e.key.keysym.scancode] = false;
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                if (!started) { started = true; Game::start(); }
                else Game::anyKey();
            }
            if (e.type != SDL_KEYDOWN) continue;
            const SDL_Keycode k = e.key.keysym.sym;
            if (!e.key.repeat && (k == SDLK_F11 || (k == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)))) {
                fullscreen = !fullscreen;
                SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                continue;
            }
            if (k == SDLK_ESCAPE) { if (e.key.repeat) continue; if (!started) quit = true; else started = false; continue; }
            if (!started) { if (!e.key.repeat) { started = true; Game::start(); } continue; }
            if (Game::anyKey()) continue;
            if (e.key.repeat) continue;
            held[e.key.keysym.scancode] = true;
            char c = 0;
            switch (k) {
            case SDLK_p: c = 'p'; break;
            case SDLK_m: c = 'm'; break;
            case SDLK_r: c = 'r'; break;
            case SDLK_t: c = 't'; break;
            case SDLK_x: c = 'x'; break;
            case SDLK_LEFTBRACKET: case SDLK_PAGEUP: c = '['; break;
            case SDLK_RIGHTBRACKET: case SDLK_PAGEDOWN: c = ']'; break;
            default: break;
            }
            if (c) Game::command(c);
        }

        const Uint64 now = SDL_GetPerformanceCounter();
        acc += std::min(0.1, (double)(now - last) / (double)SDL_GetPerformanceFrequency());
        last = now;
        constexpr double STEP = 1.0 / 60.0;
        while (acc >= STEP) {
            acc -= STEP;
            if (!started) continue;
            const bool* ks = held;
            int bits = 0;
            if (ks[SDL_SCANCODE_LEFT] || ks[SDL_SCANCODE_A]) bits |= Game::IN_L;
            if (ks[SDL_SCANCODE_RIGHT] || ks[SDL_SCANCODE_D]) bits |= Game::IN_R;
            if (ks[SDL_SCANCODE_UP] || ks[SDL_SCANCODE_W] || ks[SDL_SCANCODE_SPACE] || ks[SDL_SCANCODE_Z] || ks[SDL_SCANCODE_K]) bits |= Game::IN_J;
            Game::step(bits);
        }
        const double t = SDL_GetTicks() / 1000.0;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        canvas.begin(ImGui::GetBackgroundDrawList(), io.DisplaySize, 960, 540);
        if (!started) splash(canvas, cursed, t);
        else Game::draw(canvas, t * 1000.0);
        canvas.end();

        ImGui::Render();
        int w, h;
        SDL_GL_GetDrawableSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.008f, 0.024f, 0.067f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!capture.empty() && t >= captureAt) {
            std::vector<unsigned char> px((size_t)w * h * 4), flip((size_t)w * h * 4);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            for (int y = 0; y < h; ++y) std::copy_n(&px[(size_t)(h - 1 - y) * w * 4], (size_t)w * 4, &flip[(size_t)y * w * 4]);
            for (size_t i = 3; i < flip.size(); i += 4) flip[i] = 255;
            if (SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(flip.data(), w, h, 32, w * 4, SDL_PIXELFORMAT_ABGR8888)) {
                SDL_SaveBMP(s, capture.c_str());
                SDL_FreeSurface(s);
            }
            quit = true;
        }
        SDL_GL_SwapWindow(win);
    }

    Synth::close();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
