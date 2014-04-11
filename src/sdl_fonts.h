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
#ifndef SDL_FONTS_H
#define SDL_FONTS_H

#include "SDL_ttf.h"
#include <memory>

using SdlFont = std::shared_ptr<TTF_Font>;

enum class FontType {SMALL, MEDIUM, LARGE, _last};

// Must call this before any other SDL text functions will work.  Return false
// if there was an error loading fonts.
bool sdlFontsInit();

const SdlFont & sdlGetFont(FontType size);

#endif
