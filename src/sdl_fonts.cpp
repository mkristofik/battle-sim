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
#include "sdl_helper.h"

#include "boost/lexical_cast.hpp"
#include "boost/tokenizer.hpp"

#include <cassert>
#include <iostream>

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

int sdlDrawText(const SdlFont &font, const char *txt, SDL_Rect pos,
                const SDL_Color &color, Justify just)
{
    auto lines = sdlWordWrap(font, txt, pos.w);
    if (lines.empty()) return 0;

    int numRendered = 0;
    SdlSetClipRect(pos, [&]
    {
        auto yPos = pos.y;
        for (const auto &str : lines) {
            auto textImg = sdlRenderText(font, str.c_str(), color);

            auto xPos = pos.x;
            if (just == Justify::CENTER) {
                xPos += (pos.w / 2 - textImg->w / 2);
            }
            else if (just == Justify::RIGHT) {
                xPos += pos.w - textImg->w;
            }

            sdlBlit(textImg, xPos, yPos);
            yPos += sdlLineHeight(font);
            ++numRendered;
        }
    });

    return numRendered;
}

int sdlDrawText(const SdlFont &font, const std::string &txt, SDL_Rect pos,
                const SDL_Color &color, Justify just)
{
    return sdlDrawText(font, txt.c_str(), pos, color, just);
}

SdlSurface sdlRenderText(const SdlFont &font, const char *txt,
                         const SDL_Color &color)
{
    auto textImg = make_surface(TTF_RenderText_Blended(font.get(), txt, color));
    if (textImg == nullptr) {
        std::cerr << "Warning: error pre-rendering text: " << TTF_GetError() <<
            '\n';
    }
    return textImg;
}

SdlSurface sdlRenderText(const SdlFont &font, const std::string &txt,
                         const SDL_Color &color)
{
    return sdlRenderText(font, txt.c_str(), color);
}

SdlSurface sdlRenderText(const SdlFont &font, int number,
                         const SDL_Color &color)
{
    return sdlRenderText(font, boost::lexical_cast<std::string>(number), color);
}

int sdlLineHeight(const SdlFont &font)
{
    // The descender on lowercase g tends to run into the next line.  Increase
    // the spacing to allow for it.
    return TTF_FontLineSkip(font.get()) + 1;
}

std::vector<std::string> sdlWordWrap(const SdlFont &font,
                                     const std::string &txt, int lineLen)
{
    assert(lineLen > 0);
    if (txt.empty()) return {};

    // First try: does it all fit on one line?
    int width = 0;
    if (TTF_SizeText(font.get(), txt.c_str(), &width, 0) < 0) {
        std::cerr << "Warning: problem rendering word wrap - " <<
            TTF_GetError();
        return {txt};
    }
    if (width <= lineLen) {
        return {txt};
    }

    // Break up the string on spaces.
    boost::char_separator<char> sep{" "};
    boost::tokenizer<boost::char_separator<char>> tokens{txt, sep};

    // Doesn't matter if the first token fits, render it anyway.
    auto tok = std::begin(tokens);
    std::string strSoFar = *tok;
    ++tok;

    // Test rendering each word until we surpass the line length.
    std::vector<std::string> lines;
    while (tok != std::end(tokens)) {
        std::string nextStr = strSoFar + " " + *tok;
        TTF_SizeText(font.get(), nextStr.c_str(), &width, 0);
        if (width > lineLen) {
            lines.push_back(strSoFar);
            strSoFar = *tok;
        }
        else {
            strSoFar = nextStr;
        }
        ++tok;
    }

    // Add remaining words.
    lines.push_back(strSoFar);
    return lines;
}
