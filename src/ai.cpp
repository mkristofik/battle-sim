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

#include "ai.h"

#include "Action.h"
#include "GameState.h"
#include <array>
#include <limits>

/* Negamax algorithm
 * need a top-level function to control depth and pick an action to execute
 * have an _impl function that recurses and just returns score
 * maybe just combine these two
 * need a way to determine if one score is better than another
 * - evaluate the score array for the active team
 * - winning is good, winning with more units left is better
 * - losing is bad, losing while doing as much damage as possible is ok
 *
 * Algorithm goes like this:
 * - check the score.  If somebody just lost, return.
 * - for each possible action:
 *     + make a copy of the game state
 *     + execute it
 *     + recurse, get the ultimate score
 *     + best action is the one that ultimately gives the best score
 *     + if more than one action yields the same score, pick at random
 */
/*
 * source: http://en.wikipedia.org/wiki/Alpha-beta_pruning
 * function alphabeta(node, depth, a, ß, maximizingPlayer)
 *   if depth = 0 or node is a terminal node
 *       return the heuristic value of node
 *   if maximizingPlayer
 *       for each child of node
 *           a := max(a, alphabeta(child, depth - 1, a, ß, FALSE))
 *           if ß <= a
 *               break (* ß cut-off *)
 *       return a
 *   else
 *       for each child of node
 *           ß := min(ß, alphabeta(child, depth - 1, a, ß, TRUE))
 *           if ß <= a
 *               break (* a cut-off *)
 *       return ß
 *  (* Initial call *)
 *  alphabeta(origin, depth, -inf, +inf, TRUE)
 */

// Evaluate the game score from the given team's perspective.  Positive values
// indicate that side is winning.
int evalScore(const std::array<int, 2> &score, int team)
{
    if (team == 0) {
        return score[0] - score[1];
    }
    return score[1] - score[0];
}

Action aiNaive(const GameState &state)
{
    auto possibleActions = state.getPossibleActions();
    Action *best = nullptr;
    int bestScore = std::numeric_limits<int>::min();

    for (auto &a : possibleActions) {
        GameState gsCopy{state};
        a.damage = gsCopy.getSimulatedDamage(a);

        // TODO: this duplicates the logic of how moves are executed
        gsCopy.execute(a);
        if (gsCopy.isRetaliationAllowed(a)) {
            auto retal = gsCopy.makeRetaliation(a);
            gsCopy.execute(retal);
        }

        // Note: use the main game state's active team, not the copy!
        int scoreDiff = evalScore(gsCopy.getScore(), state.getActiveTeam());

        if (scoreDiff > bestScore) {
            bestScore = scoreDiff;
            best = &a;
        }
    }

    if (!best) return {};
    best->damage = 0;  // clear simulated damage
    return *best;
}
