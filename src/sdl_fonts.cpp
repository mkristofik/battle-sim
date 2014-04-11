/*
    Copyright (C) 2013-2014 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "sdl_fonts.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace
{
    SdlFont loadFont(const char *filename, int ptSize)
    {
        SdlFont font(TTF_OpenFont(filename, ptSize), TTF_CloseFont);
        if (!font) {
            std::cerr << "Error loading font " << filename << " size " << ptSize
                << "\n    " << TTF_GetError() << '\n';
        }
        return font;
    }

    std::vector<SdlFont> fonts;
}

bool sdlFontsInit()
{
    if (TTF_Init() < 0) return false;
    atexit(TTF_Quit);

    // Note: do this in enum order.
    fonts.push_back(loadFont("../DejaVuSans.ttf", 9));
    fonts.push_back(loadFont("../DejaVuSans.ttf", 12));
    fonts.push_back(loadFont("../DejaVuSans.ttf", 14));
    assert(fonts.size() == static_cast<unsigned>(FontType::_last));
    atexit([&] { fonts.clear(); });

    return true;
}

const SdlFont & sdlGetFont(FontType size)
{
    return fonts[static_cast<int>(size)];
}
