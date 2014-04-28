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
#include <string>
#include <vector>

using SdlFont = std::shared_ptr<TTF_Font>;
using SdlSurface = std::shared_ptr<SDL_Surface>;

enum class FontType {SMALL, MEDIUM, LARGE, _last};
enum class Justify {LEFT, CENTER, RIGHT};

// Must call this before any other SDL text functions will work.  Return false
// if there was an error loading fonts.
bool sdlFontsInit();

const SdlFont & sdlGetFont(FontType size);

// Draw text to the screen.  Return the number of word-wrapped lines of text.
int sdlDrawText(const SdlFont &font, const char *txt, SDL_Rect pos,
                const SDL_Color &color, Justify just = Justify::LEFT);
int sdlDrawText(const SdlFont &font, const std::string &txt, SDL_Rect pos,
                const SDL_Color &color, Justify just = Justify::LEFT);

// Create an image containing some text to be drawn later.  No word-wrapping is
// done.
SdlSurface sdlRenderText(const SdlFont &font, const char *txt,
                         const SDL_Color &color);
SdlSurface sdlRenderText(const SdlFont &font, const std::string &txt,
                         const SDL_Color &color);
SdlSurface sdlRenderText(const SdlFont &font, int number,
                         const SDL_Color &color);

// Return the default space between lines of text.
int sdlLineHeight(const SdlFont &font);

// Break a string into multiple lines based on rendered line length.
// Return one string per line.
std::vector<std::string> sdlWordWrap(const SdlFont &font,
                                     const std::string &txt, int lineLen);

#endif
