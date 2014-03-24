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

#include "ai.h"

#include "Action.h"
#include "GameState.h"
#include <array>
#include <limits>

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

// AI functions return the difference in final score (or score when the search
// stops) of executing the best moves for both sides.  Positive values good for
// team 0, negative values good for team 1.

int noLookAhead(const GameState &gs)
{
    auto score = gs.getScore();
    return score[0] - score[1];
}

// source: http://en.wikipedia.org/wiki/Alpha-beta_pruning
int alphaBeta(const GameState &gs, int depth, int alpha, int beta)
{
    // If we've run out of search time or the game has ended, stop.
    auto score = gs.getScore();
    if (score[0] == 0 || score[1] == 0) {
        return (score[0] - score[1]) * 4;  // Place an emphasis on winning.
    }
    else if (depth <= 0) {
        return score[0] - score[1];
    }

    for (auto &action : gs.getPossibleActions()) {
        GameState gsCopy{gs};
        gsCopy.runActionSeq(action);
        gsCopy.nextTurn();

        int finalScore = alphaBeta(gsCopy, depth - 1, alpha, beta);
        if (gs.getActiveTeam() == 0) {
            alpha = std::max(alpha, finalScore);
            if (beta <= alpha) break;
        }
        else {
            beta = std::min(beta, finalScore);
            if (beta <= alpha) break;
        }
    }

    return (gs.getActiveTeam() == 0) ? alpha : beta;
}

template <typename F>
Action bestAction(const GameState &gs, F aiFunc)
{
    auto possibleActions = gs.getPossibleActions();
    Action *best = nullptr;
    int bestScore = std::numeric_limits<int>::min();

    for (auto &action : possibleActions) {
        GameState gsCopy{gs};
        gsCopy.runActionSeq(action);
        gsCopy.nextTurn();

        int scoreDiff = aiFunc(gsCopy);
        if (gs.getActiveTeam() != 0) scoreDiff = -scoreDiff;

        if (scoreDiff > bestScore) {
            bestScore = scoreDiff;
            best = &action;
        }
    }

    if (!best) {
        std::cout << "    No action is good, skipping turn.\n";
        return {};
    }
    if (best->type != ActionType::EFFECT) {
        best->damage = 0;  // clear simulated damage
    }
    return *best;
}

Action minimax(const GameState &gs, int searchDepth)
{
    auto abSearch = [&] (const GameState &gs) {
        return alphaBeta(gs,
                         searchDepth,
                         std::numeric_limits<int>::min(),
                         std::numeric_limits<int>::max());
    };
    return bestAction(gs, abSearch);
}

Action aiNaive(GameState gs)
{
    gs.setSimMode();
    return bestAction(gs, noLookAhead);
}

Action aiBetter(GameState gs)
{
    gs.setSimMode();
    return minimax(gs, 3);
}

Action aiBest(GameState gs)
{
    // TODO: can we do this with iterative deepening?  Run successively longer
    // search depths up to 10 seconds perhaps?  We might consider stopping if
    // the first few runs all produce the same move.
    gs.setSimMode();
    return minimax(gs, 7);
}
