/*
    Copyright (C) 2013 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef SDL_HELPER_H
#define SDL_HELPER_H

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"
#include "hex_utils.h"
#include <iostream>
#include <memory>
#include <string>
#include <utility>

using SdlSurface = std::shared_ptr<SDL_Surface>;
using SdlFont = std::shared_ptr<TTF_Font>;
using SdlMusic = std::shared_ptr<Mix_Music>;
using SdlSound = std::shared_ptr<Mix_Chunk>;

// Must call this before any other SDL functions will work.  There is no
// recovery if this returns false (you should exit the program).
bool sdlInit(Sint16 winWidth, Sint16 winHeight, const char *iconFile,
             const char *caption);

// Like std::make_shared, but with SDL_Surface.
SdlSurface make_surface(SDL_Surface *surf);

SdlSurface sdlDeepCopy(const SdlSurface &src);

// Flip a surface or sprite sheet.  Creates a new surface.
SdlSurface sdlFlipH(const SdlSurface &src);
SdlSurface sdlFlipSheetH(const SdlSurface &src, int numFrames);

// Set the alpha channel of all non-transparent pixels (because SDL_SetAlpha
// doesn't work for images that already have per-pixel alpha channel).
SdlSurface sdlSetAlpha(const SdlSurface &src, double alpha);

// Draw the full surface to the screen using (px,py) as the upper-left corner.
// Use the raw SDL_BlitSurface if you need something more specific.
void sdlBlit(const SdlSurface &surf, Sint16 px, Sint16 py);
void sdlBlit(const SdlSurface &surf, const Point &pos);

// Draw a portion of a sprite sheet to the screen.  Assumes each frame is the
// size of a hex.
void sdlBlitFrame(const SdlSurface &surf, int frame, Sint16 px, Sint16 py);
void sdlBlitFrame(const SdlSurface &surf, int frame, const Point &pos);

// Clear the given region of the screen.
void sdlClear(SDL_Rect region);

// Set the clipping region for the duration of a lambda or other function call.
struct SdlSetClipRect
{
    SDL_Rect original_;
    SDL_Surface *screen_;

    template <typename Func>
    SdlSetClipRect(const SDL_Rect &rect, const Func &f) : 
        screen_{SDL_GetVideoSurface()}
    {
        SDL_GetClipRect(screen_, &original_);
        SDL_SetClipRect(screen_, &rect);
        f();
    }

    ~SdlSetClipRect()
    {
        SDL_SetClipRect(screen_, &original_);
    }
};

// Lock an image before accessing the underlying pixels.
struct SdlLock
{
    SDL_Surface *surface_;

    template <typename Func>
    SdlLock(SdlSurface &surf, const Func &f) : surface_{surf.get()}
    {
        if (SDL_MUSTLOCK(surface_)) {
            if (SDL_LockSurface(surface_) == 0) {
                f();
            }
            else {
                std::cerr << "Error locking surface: " << SDL_GetError()
                    << '\n';
            }
        }
        else {
            f();
        }
    }

    ~SdlLock()
    {
        if (SDL_MUSTLOCK(surface_)) {
            SDL_UnlockSurface(surface_);
        }
    }
};

// Load a resource from disk.  Returns null on failure.
SdlSurface sdlLoadImage(const char *filename);
SdlSurface sdlLoadImage(const std::string &filename);
SdlFont sdlLoadFont(const char *filename, int ptSize);
SdlMusic sdlLoadMusic(const char *filename);
SdlMusic sdlLoadMusic(const std::string &filename);
SdlSound sdlLoadSound(const char *filename);
// note: don't try to allocate these at global scope.  They need sdlInit()
// before they will work, and the objects must be freed before SDL teardown
// happens.

// Draw a dashed line to the screen starting at (px,py).
void sdlDashedLineH(Sint16 px, Sint16 py, Uint16 len, Uint32 color);
void sdlDashedLineV(Sint16 px, Sint16 py, Uint16 len, Uint32 color);

// Rectangle functions.
bool insideRect(Sint16 x, Sint16 y, const SDL_Rect &rect);
std::pair<double, double> rectPct(Sint16 x, Sint16 y, const SDL_Rect &rect);
Dir8 nearEdge(Sint16 x, Sint16 y, const SDL_Rect &rect);

// Return the bounding box for the given image.
SDL_Rect sdlGetBounds(const SdlSurface &surf, Sint16 x, Sint16 y);

// Draw text to the screen.  Return the number of word-wrapped lines of text.
enum class Justify {LEFT, CENTER};
int sdlDrawText(const SdlFont &font, const char *txt, SDL_Rect pos,
                const SDL_Color &color, Justify just = Justify::LEFT);
int sdlDrawText(const SdlFont &font, const std::string &txt, SDL_Rect pos,
                const SDL_Color &color, Justify just = Justify::LEFT);

// Create an image containing some text to be drawn later.  No word-wrapping is
// done.
SdlSurface sdlPreRender(const SdlFont &font, const char *txt,
                        const SDL_Color &color);
SdlSurface sdlPreRender(const SdlFont &font, const std::string &txt,
                        const SDL_Color &color);
SdlSurface sdlPreRender(const SdlFont &font, int number,
                        const SDL_Color &color);

// Play sound files at a reasonable volume.
void sdlPlayMusic(const SdlMusic &music);
void sdlPlaySound(const SdlSound &Sound);

// Common colors.
const SDL_Color WHITE = SDL_Color{255, 255, 255, 0};

#endif
