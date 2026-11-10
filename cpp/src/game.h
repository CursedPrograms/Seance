#pragma once
// game.h - SÉANCE's rules and drawing, ported from the browser game (site/game.js).
#include "canvas.h"

namespace Game {
    enum { IN_L = 1, IN_R = 2, IN_J = 4 };

    void init(const char* saveFolder);     // the solved rooms and best times are kept there
    void start();                          // from the splash: the first room not yet solved
    bool anyKey();                         // the "any key" after a solved room / the end; true: used up
    bool command(char k);                  // p m r t x [ ]: true when used
    void step(int inputBits);              // one fixed step (1/60 s)
    void draw(Canvas& c, double nowMs);
    void load(int room);                   // --room N: straight into a room (for testing)
}
