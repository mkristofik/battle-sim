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
#include "sdl_helper.h"

#include "algo.h"

#include "SDL_rotozoom.h"

#include "boost/filesystem.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    SDL_Surface *screen = nullptr;

    using DashSize = std::pair<Sint16, Uint16>;  // line-relative pos, width
    std::vector<DashSize> dashedLine(Uint16 lineLen)
    {
        const Uint16 spaceSize = 6;
        const Uint16 dashSize = spaceSize * 3 / 2;

        std::vector<DashSize> dashes;
        Sint16 pos = 0;
        while (pos < lineLen) {
            if (pos + dashSize < lineLen) {
                dashes.emplace_back(pos, dashSize);
                pos += dashSize + spaceSize;
            }
            else {
                dashes.emplace_back(pos, lineLen - pos);
                break;
            }
        }

        return dashes;
    }

    // source: Battle for Wesnoth, flip_surface() in sdl_utils.cpp.
    void flipH(SdlSurface &src, int frameStart, int frameWidth)
    {
        auto pixels = static_cast<Uint32 *>(src->pixels);
        for (auto y = 0; y < src->h; ++y) {
            for (auto x = 0; x < frameWidth / 2; ++x) {
                auto i1 = y * src->w + frameStart + x;
                auto i2 = y * src->w + frameStart + frameWidth - x - 1;
                std::swap(pixels[i1], pixels[i2]);
            }
        }
    }

    // Convert the given surface to the screen format.  Return a null surface
    // on failure.
    SdlSurface sdlDisplayFormat(const SdlSurface &src)
    {
        auto surf = make_surface(SDL_DisplayFormatAlpha(src.get()));
        if (!surf) {
            std::cerr << "Error converting to display format: " << SDL_GetError()
                << '\n';
        }
        return surf;
    }

    // Get the full path to an image file.
    std::string getImagePath(const char *filename)
    {
        boost::filesystem::path imagePath{"../img"};
        imagePath /= filename;
        return imagePath.string();
    }
}

bool sdlInit(Sint16 winWidth, Sint16 winHeight, const char *iconFile,
             const char *caption)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1) {
        std::cerr << "Error initializing SDL: " << SDL_GetError();
        return false;
    }
    atexit(SDL_Quit);

    if (IMG_Init(IMG_INIT_PNG) < 0) {
        std::cerr << "Error initializing SDL_image: " << IMG_GetError();
        return false;
    }
    atexit(IMG_Quit);

    if (!sdlFontsInit()) {
        std::cerr << "Error initializing SDL_ttf: " << TTF_GetError();
        return false;
    }

    if (Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG) < 0) {
        std::cerr << "Warning: error initializing SDL_mixer: " << Mix_GetError();
        // not a fatal error
    }
    atexit(Mix_Quit);

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        std::cerr << "Warning: error opening SDL_mixer: " << Mix_GetError();
        // not a fatal error
    }
    atexit(Mix_CloseAudio);

    // Have to do this prior to SetVideoMode.
    auto icon = make_surface(IMG_Load(getImagePath(iconFile).c_str()));
    if (icon != nullptr) {
        SDL_WM_SetIcon(icon.get(), nullptr);
    }
    else {
        std::cerr << "Warning: error loading icon file: " << IMG_GetError();
    }

    screen = SDL_SetVideoMode(winWidth, winHeight, 0, SDL_SWSURFACE);
    if (screen == nullptr) {
        std::cerr << "Error setting video mode: " << SDL_GetError();
        return false;
    }

    SDL_WM_SetCaption(caption, "");
    return true;
}

SdlSurface make_surface(SDL_Surface *surf)
{
    return SdlSurface(surf, SDL_FreeSurface);
}

SdlSurface sdlCreate(int width, int height)
{
    assert(screen != nullptr);
    auto surf = SDL_CreateRGBSurface(screen->flags,
                                     width,
                                     height,
                                     screen->format->BitsPerPixel,
                                     screen->format->Rmask,
                                     screen->format->Gmask,
                                     screen->format->Bmask,
                                     screen->format->Amask);
    if (surf == nullptr) {
        std::cerr << "Error creating blank surface: " << SDL_GetError();
        return {};
    }

    return make_surface(surf);
}

SdlSurface sdlDeepCopy(const SdlSurface &src)
{
    auto surf = SDL_ConvertSurface(src.get(), src->format, src->flags);
    if (!surf) {
        std::cerr << "Error copying surface: " << SDL_GetError() << '\n';
        return nullptr;
    }
    return make_surface(surf);
}

SdlSurface sdlFlipH(const SdlSurface &src)
{
    auto surf = sdlDeepCopy(src);
    if (!surf) {
        return nullptr;
    }

    SdlLock(surf, [&] {
        flipH(surf, 0, surf->w);
    });
    return surf;
}

SdlSurface sdlFlipSheetH(const SdlSurface &src, int numFrames)
{
    assert(src->w % numFrames == 0);  // verify each frame is the same size
    // TODO: throwing an exception here might be better.

    auto surf = sdlDeepCopy(src);
    if (!surf) {
        return nullptr;
    }

    auto frameWidth = surf->w / numFrames;
    SdlLock(surf, [&] {
        for (auto x = 0; x < surf->w; x += frameWidth) {
            flipH(surf, x, frameWidth);
        }
    });
    return surf;
}

SdlSurface sdlRotate(const SdlSurface &src, double angle_rad)
{
    double angle_deg = angle_rad * 180.0 / M_PI;
    return make_surface(rotozoomSurface(src.get(), angle_deg, 1, SMOOTHING_ON));
}

SdlSurface sdlZoom(const SdlSurface &src, double zoom)
{
    return make_surface(rotozoomSurface(src.get(), 0, zoom, SMOOTHING_ON));
}

SdlSurface sdlSetAlpha(const SdlSurface &src, double alpha)
{
    auto surf = sdlDeepCopy(src);
    if (!surf) {
        return nullptr;
    }

    SdlLock(surf, [&] {
        auto *begin = static_cast<Uint32 *>(surf->pixels);
        auto *end = begin + surf->w * surf->h;
        auto *i = begin;
        while (i != end) {
            auto curAlpha = (*i & surf->format->Amask) >> surf->format->Ashift;
            if (curAlpha != SDL_ALPHA_TRANSPARENT) {
                Uint8 newAlpha = curAlpha * bound(alpha, 0.0, 1.0);
                *i &= ~surf->format->Amask;
                *i |= (newAlpha << surf->format->Ashift);
            }
            ++i;
        }
    });

    return surf;
}

SdlSurface sdlBlendColor(const SdlSurface &src, const SDL_Color &color,
                         double ratio)
{
    ratio = bound(ratio, 0.0, 1.0);
    auto surf = sdlDeepCopy(src);
    if (!surf) {
        return nullptr;
    }

    SdlLock(surf, [&] {
        auto *begin = static_cast<Uint32 *>(surf->pixels);
        auto *end = begin + surf->w * surf->h;
        for (auto *i = begin; i != end; ++i) {
            auto curAlpha = (*i & surf->format->Amask) >> surf->format->Ashift;
            if (curAlpha == SDL_ALPHA_TRANSPARENT) continue;

            int curRed = (*i & surf->format->Rmask) >> surf->format->Rshift;
            int curGreen = (*i & surf->format->Gmask) >> surf->format->Gshift;
            int curBlue = (*i & surf->format->Bmask) >> surf->format->Bshift;
            Uint32 newRed = curRed + (color.r - curRed) * ratio;
            Uint32 newGreen = curGreen + (color.g - curGreen) * ratio;
            Uint32 newBlue = curBlue + (color.b - curBlue) * ratio;

            *i &= surf->format->Amask;
            *i |= (newRed << surf->format->Rshift);
            *i |= (newGreen << surf->format->Gshift);
            *i |= (newBlue << surf->format->Bshift);
        }
    });

    return surf;
}

void sdlBlit(const SdlSurface &surf, Sint16 px, Sint16 py)
{
    assert(screen != nullptr);
    SDL_Rect dest = {px, py, 0, 0};
    if (SDL_BlitSurface(surf.get(), nullptr, screen, &dest) < 0) {
        std::cerr << "Warning: error drawing to screen: " << SDL_GetError()
            << '\n';
    }
}

void sdlBlit(const SdlSurface &surf, const Point &pos)
{
    sdlBlit(surf, pos.x, pos.y);
}

void sdlBlitFrame(const SdlSurface &surf, int frame, int numFrames,
                  Sint16 px, Sint16 py)
{
    assert(screen != nullptr);
    int pFrameWidth = surf->w / numFrames;
    SDL_Rect src;
    src.x = frame * pFrameWidth;
    src.y = 0;
    src.w = pFrameWidth;
    src.h = surf->h;
    auto dest = SDL_Rect{px, py, 0, 0};
    if (SDL_BlitSurface(surf.get(), &src, screen, &dest) < 0) {
        std::cerr << "Warning: error drawing to screen: " << SDL_GetError()
            << '\n';
    }
}

void sdlBlitFrame(const SdlSurface &surf, int frame, int numFrames,
                  const Point &pos)
{
    sdlBlitFrame(surf, frame, numFrames, pos.x, pos.y);
}

void sdlClear(SDL_Rect region)
{
    assert(screen != nullptr);
    auto black = SDL_MapRGB(screen->format, 0, 0, 0);
    if (SDL_FillRect(screen, &region, black) < 0) {
        std::cerr << "Error clearing screen region: " << SDL_GetError() << '\n';
    }
}

SdlSurface sdlRenderBorder(int width, int height)
{
    auto surf = sdlCreate(width, height);
    auto bgColor = SDL_MapRGB(screen->format, BORDER_BG.r, BORDER_BG.g,
                               BORDER_BG.b);
    auto fgColor = SDL_MapRGB(screen->format, BORDER_FG.r, BORDER_FG.g,
                               BORDER_FG.b);

    // Draw a dark border 5 pixels wide around the outside edge.
    SDL_Rect top = {0};
    top.w = width;
    top.h = 5;
    SDL_FillRect(surf.get(), &top, bgColor);
    SDL_Rect left = {0};
    left.w = 5;
    left.h = height;
    SDL_FillRect(surf.get(), &left, bgColor);
    SDL_Rect bottom = {0};
    bottom.y = height - 5;
    bottom.w = width;
    bottom.h = 5;
    SDL_FillRect(surf.get(), &bottom, bgColor);
    SDL_Rect right = {0};
    right.x = width - 5;
    right.w = 5;
    right.h = height;
    SDL_FillRect(surf.get(), &right, bgColor);

    // Inlay a lighter line in the center of the border.
    top.x = 2;
    top.y = 2;
    top.w = width - 4;
    top.h = 1;
    SDL_FillRect(surf.get(), &top, fgColor);
    left.x = 2;
    left.y = 2;
    left.w = 1;
    left.h = height - 4;
    SDL_FillRect(surf.get(), &left, fgColor);
    bottom.x = 2;
    bottom.y = height - 3;
    bottom.w = width - 4;
    bottom.h = 1;
    SDL_FillRect(surf.get(), &bottom, fgColor);
    right.x = width - 3;
    right.y = 2;
    right.w = 1;
    right.h = height - 4;
    SDL_FillRect(surf.get(), &right, fgColor);

    return surf;
}

SdlSurface sdlLoadImage(const char *filename)
{
    assert(SDL_WasInit(SDL_INIT_VIDEO) != 0);

    auto img = make_surface(IMG_Load(getImagePath(filename).c_str()));
    if (!img) {
        std::cerr << "Error loading image " << filename
            << "\n    " << IMG_GetError() << '\n';
        return img;
    }
    return sdlDisplayFormat(img);
}

SdlSurface sdlLoadImage(const std::string &filename)
{
    return sdlLoadImage(filename.c_str());
}

SdlMusic sdlLoadMusic(const char *filename)
{
    SdlMusic music(Mix_LoadMUS(filename), Mix_FreeMusic);
    if (!music) {
        std::cerr << "Error loading music " << filename << "\n    "
            << Mix_GetError() << '\n';
    }
    return music;
}

SdlMusic sdlLoadMusic(const std::string &filename)
{
    return sdlLoadMusic(filename.c_str());
}

SdlSound sdlLoadSound(const char *filename)
{
    SdlSound sound{Mix_LoadWAV(filename), Mix_FreeChunk};
    if (!sound) {
        std::cerr << "Error loading sound " << filename << "\n    "
            << Mix_GetError() << '\n';
    }
    return sound;
}

void sdlDashedLineH(Sint16 px, Sint16 py, Uint16 len, Uint32 color)
{
    assert(screen != nullptr);
    const Uint16 lineWidth = 1;
    for (const auto &dash : dashedLine(len)) {
        SDL_Rect r = {static_cast<Sint16>(px + dash.first), py, dash.second,
                      lineWidth};
        if (SDL_FillRect(screen, &r, color) < 0) {
            std::cerr << "Error drawing horizontal dashed line: "
                << SDL_GetError() << '\n';
            return;
        }
        // TODO: this could be a unit test.
        //std::cout << 'H' << r.x << ',' << r.y << 'x' << r.w << '\n';
    }
}

void sdlDashedLineV(Sint16 px, Sint16 py, Uint16 len, Uint32 color)
{
    assert(screen != nullptr);
    const Uint16 lineWidth = 1;
    for (const auto &dash : dashedLine(len)) {
        SDL_Rect r = {px, static_cast<Sint16>(py + dash.first), lineWidth,
                      dash.second};
        if (SDL_FillRect(screen, &r, color) < 0) {
            std::cerr << "Error drawing horizontal dashed line: "
                << SDL_GetError() << '\n';
            return;
        }
        // TODO: this could be a unit test.
        //std::cout << 'V' << r.x << ',' << r.y << 'x' << r.h << '\n';
    }
}

bool insideRect(Sint16 x, Sint16 y, const SDL_Rect &rect)
{
    return x >= rect.x &&
           y >= rect.y &&
           x < rect.x + rect.w &&
           y < rect.y + rect.h;
}

std::pair<double, double> rectPct(Sint16 x, Sint16 y, const SDL_Rect &rect)
{
    return {(static_cast<double>(x) - rect.x) / (rect.w - 1),
            (static_cast<double>(y) - rect.y) / (rect.h - 1)};
}

Dir8 nearEdge(Sint16 x, Sint16 y, const SDL_Rect &rect)
{
    if (!insideRect(x, y, rect)) {
        return Dir8::None;
    }

    const Sint16 edgeDist = 10;

    if (x - rect.x < edgeDist) {  // left edge
        if (y - rect.y < edgeDist) {
            return Dir8::NW;
        }
        else if (rect.y + rect.h - 1 - y < edgeDist) {
            return Dir8::SW;
        }
        else {
            return Dir8::W;
        }
    }
    else if (rect.x + rect.w - 1 - x < edgeDist) {  // right edge
        if (y - rect.y < edgeDist) {
            return Dir8::NE;
        }
        else if (rect.y + rect.h - 1 - y < edgeDist) {
            return Dir8::SE;
        }
        else {
            return Dir8::E;
        }
    }
    else if (y - rect.y < edgeDist) {  // top edge
        return Dir8::N;
    }
    else if (rect.y + rect.h - 1 - y < edgeDist) {
        return Dir8::S;
    }

    return Dir8::None;
}

SDL_Rect sdlGetBounds(const SdlSurface &surf, Sint16 x, Sint16 y)
{
    return {x, y, static_cast<Uint16>(surf->w), static_cast<Uint16>(surf->h)};
}

void sdlPlayMusic(const SdlMusic &music)
{
    if (Mix_PlayMusic(music.get(), 0) == -1) {
        std::cerr << "Error playing music: " << Mix_GetError() << '\n';
        return;
    }

    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
}

void sdlPlaySound(const SdlSound &sound)
{
    auto channel = Mix_PlayChannel(-1, sound.get(), 0);
    if (channel == -1) {
        std::cerr << "Error playing sound: " << Mix_GetError() << '\n';
        return;
    }

    Mix_Volume(channel, MIX_MAX_VOLUME / 3);
}
