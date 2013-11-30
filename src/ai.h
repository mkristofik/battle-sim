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
#ifndef AI_H
#define AI_H

class Action;
class GameState;

// Simulate all possible actions, choose the one that most improves the score
// for the active team.
Action aiNaive(const GameState &state);

#endif
