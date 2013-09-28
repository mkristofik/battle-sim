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
#include "Action.h"
#include "Anim.h"
#include "Battlefield.h"
#include "GameState.h"
#include "LogView.h"
#include "Pathfinder.h"
#include "Unit.h"
#include "UnitType.h"
#include "algo.h"
#include "hex_utils.h"
#include "sdl_helper.h"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: ideas
// - make a bigger window with some portraits to indicate heroes
//      * need some scaling because the portraits are big
// - leave space to write unit stats for the highlighted hex

// TODO: things we need to figure all this out
// - traits (X macros?)

namespace
{
    std::unique_ptr<Battlefield> bf;
    std::unique_ptr<LogView> logv;
    SDL_Rect bfWindow = {0, 0, 288, 360};
    SDL_Rect logWindow = {0, 360, 288, 60};
    SdlFont labelFont;
    std::unordered_map<std::string, int> mapUnitPos;
    std::deque<std::unique_ptr<Anim>> anims;
    bool actionTaken = false;
    int roundNum = 1;
    bool logHasFocus = false;  // battlefield has focus by default
    bool gameOver = false;  // only certain actions allowed after game ends

    // Unit placement on the grid.
    // team 1 on the left, team 2 on the right
    const Point unitPos[] = {{1,0}, {1,1}, {1,2}, {1,3},  // team 1 row 1
                             {0,1}, {0,2}, {0,3},         // team 1 row 2
                             {3,0}, {3,1}, {3,2}, {3,3},  // team 2 row 1
                             {4,1}, {4,2}, {4,3}};        // team 2 row 2

    // Map unit position strings used by JSON data to 'unitPos' array indexes.
    void translateUnitPos()
    {
        mapUnitPos.emplace("t1p1", 0);
        mapUnitPos.emplace("t1p2", 1);
        mapUnitPos.emplace("t1p3", 2);
        mapUnitPos.emplace("t1p4", 3);
        mapUnitPos.emplace("t1p5", 4);
        mapUnitPos.emplace("t1p6", 5);
        mapUnitPos.emplace("t1p7", 6);
        mapUnitPos.emplace("t2p1", 7);
        mapUnitPos.emplace("t2p2", 8);
        mapUnitPos.emplace("t2p3", 9);
        mapUnitPos.emplace("t2p4", 10);
        mapUnitPos.emplace("t2p5", 11);
        mapUnitPos.emplace("t2p6", 12);
        mapUnitPos.emplace("t2p7", 13);
    }

    bool parseJson(const char *filename, rapidjson::Document &doc)
    {
        boost::filesystem::path dataPath{"../data"};
        dataPath /= filename;
        std::shared_ptr<FILE> fp{fopen(dataPath.string().c_str(), "r"), fclose};

        rapidjson::FileStream file(fp.get());
        if (doc.ParseStream<0>(file).HasParseError()) {
            std::cerr << "Error reading json file " << dataPath.c_str() << ": "
                << doc.GetParseError() << '\n';
            return false;
        }
        if (!doc.IsObject()) {
            std::cerr << "Expected top-level object in json file\n";
            return false;
        }

        return true;
    }
}

// Get path from the active unit's hex to the target hex.
std::vector<int> getPathTo(int aTgt)
{
    auto emptyHexes = [&] (int aIndex) {
        std::vector<int> nbrs;
        for (auto n : bf->aryNeighbors(aIndex)) {
            if (!gs->getUnitAt(n)) {
                nbrs.push_back(n);
            }
        }
        return nbrs;
    };

    Pathfinder pf;
    pf.setNeighbors(emptyHexes);
    pf.setGoal(aTgt);
    return pf.getPathFrom(gs->getActiveUnit()->aHex);
}

// Return true if the active unit has a ranged attack and there are no enemies
// adjacent to it.
bool isRangedAttackAllowed()
{
    const auto attacker = gs->getActiveUnit();
    if (!attacker || !attacker->type->hasRangedAttack) {
        return false;
    }

    auto enemy = (attacker->team == 0) ? 1 : 0;
    for (auto n : bf->aryNeighbors(attacker->aHex)) {
        const auto defender = gs->getUnitAt(n);
        if (defender && defender->team == enemy) {
            return false;
        }
    }

    return true;
}

// Human player's function - determine what action the active unit can take if
// the user clicks at the given screen coordinates.
Action getPossibleAction(int px, int py)
{
    Action action;
    auto aTgt = bf->aryFromPixel(px, py);
    if (aTgt < 0) {
        return {};
    }
    action.attackTarget = aTgt;
    auto hTgt = bf->hexFromAry(aTgt);

    action.attacker = gs->getActiveUnit();
    if (!action.attacker) {
        return {};
    }

    action.defender = gs->getUnitAt(aTgt);
    int enemy = (action.attacker->team == 0) ? 1 : 0;

    // Pathfinder includes the current hex.
    auto moveRange = static_cast<unsigned>(action.attacker->type->moves) + 1;

    if (action.defender && action.defender->team == enemy) {
        if (isRangedAttackAllowed()) {
            action.type = ActionType::RANGED;
            return action;
        }

        action.type = ActionType::ATTACK;
        auto pOffset = Point{px, py} - pixelFromHex(hTgt);
        auto attackDir = getSector(pOffset.x, pOffset.y);
        auto hMoveTo = adjacent(hTgt, attackDir);
        auto aMoveTo = bf->aryFromHex(hMoveTo);
        action.path = getPathTo(aMoveTo);
        if (!action.path.empty() && action.path.size() <= moveRange) {
            return action;
        }
    }
    else if (!action.defender) {
        action.type = ActionType::MOVE;
        action.path = getPathTo(aTgt);
        if (action.path.size() > 1 && action.path.size() <= moveRange) {
            return action;
        }
    }

    return {};
}

// Get direction to have a unit in the source hex face the target hex.
Facing getFacing(const Point &hSrc, const Point &hTgt, Facing curFacing)
{
    auto dir = direction(hSrc, hTgt);

    if (dir == Dir::NW || dir == Dir::SW) {
        return Facing::LEFT;
    }
    if (dir == Dir::N || dir == Dir::S) {
        if (hSrc.x < hTgt.x) {
            return Facing::RIGHT;
        }
        else if (hSrc.x > hTgt.x) {
            return Facing::LEFT;
        }
        else {
            return curFacing;
        }
    }

    return Facing::RIGHT;
}

// Make the active unit carry out the given action.
void executeAction(const Action &action)
{
    if (action.type == ActionType::NONE) {
        return;
    }

    auto unit = gs->getActiveUnit();
    if (!unit) {
        return;
    }

    // All other actions might involve moving.
    if (action.path.size() > 1) {
        auto facing = unit->face;
        for (auto i = 1u; i < action.path.size(); ++i) {
            auto hSrc = bf->hexFromAry(action.path[i - 1]);
            auto hDest = bf->hexFromAry(action.path[i]);
            auto prevFacing = facing;
            facing = getFacing(hSrc, hDest, prevFacing);

            anims.emplace_back(make_unique<AnimMove>(*unit, hDest, facing));
        }
        unit->aHex = action.path.back();
        unit->face = facing;
    }

    // These animations happen after the attacker is done moving.
    if (action.type == ActionType::RANGED) {
        auto rangedSeq = make_unique<AnimParallel>();

        auto hSrc = bf->hexFromAry(unit->aHex);
        auto aTgt = action.attackTarget;
        auto hTgt = bf->hexFromAry(aTgt);

        unit->face = getFacing(hSrc, hTgt, unit->face);
        auto animShooter = make_unique<AnimRanged>(*unit, hTgt);

        auto animShot = make_unique<AnimProjectile>(unit->type->projectile,
             hSrc, hTgt, animShooter->getShotTime());

        auto defender = gs->getUnitAt(aTgt);
        defender->face = getFacing(hTgt, hSrc, defender->face);
        defender->takeDamage(action.damage);
        auto hitTime = animShooter->getShotTime() + animShot->getFlightTime();

        rangedSeq->add(std::move(animShooter));
        rangedSeq->add(std::move(animShot));
        if (defender->num > 0) {
            rangedSeq->add(make_unique<AnimDefend>(*defender, hSrc, hitTime));
        }
        else {
            rangedSeq->add(make_unique<AnimDie>(*defender, hitTime));
        }
        anims.emplace_back(std::move(rangedSeq));
    }
    else if (action.type == ActionType::ATTACK) {
        auto attackSeq = make_unique<AnimParallel>();

        auto hSrc = bf->hexFromAry(unit->aHex);
        auto hTgt = bf->hexFromAry(action.attackTarget);

        unit->face = getFacing(hSrc, hTgt, unit->face);
        auto anim1 = make_unique<AnimAttack>(*unit, hTgt);
        auto hitTime = anim1->getHitTime();

        auto defender = gs->getUnitAt(action.attackTarget);
        defender->face = getFacing(hTgt, hSrc, defender->face);
        defender->takeDamage(action.damage);

        attackSeq->add(std::move(anim1));
        if (defender->num > 0) {
            attackSeq->add(make_unique<AnimDefend>(*defender, hSrc, hitTime));
        }
        else {
            attackSeq->add(make_unique<AnimDie>(*defender, hitTime));
        }
        anims.emplace_back(std::move(attackSeq));
    }
}

void logAction(const Action &action)
{
    if (action.type != ActionType::ATTACK && action.type != ActionType::RANGED) {
        return;
    }
    if (!action.attacker || !action.defender) {
        return;
    }

    int numKilled = action.defender->simulateDamage(action.damage);

    std::ostringstream ostr("- ", std::ios::ate);
    if (action.attacker->num == 1) {
        ostr << "1 " << action.attacker->type->name << " attacks ";
    }
    else {
        ostr << action.attacker->num << ' ' << action.attacker->type->plural <<
            " attack ";
    }
    if (action.defender->num == 1) {
        ostr << "1 " << action.defender->type->name;
    }
    else {
        ostr << action.defender->num << ' ' << action.defender->type->plural;
    }
    ostr << " for " << action.damage << " damage.";
    if (numKilled == 1) {
        ostr << "  1 " << action.defender->type->name << " perishes.";
    }
    else if (numKilled > 1) {
        ostr << "  " << numKilled << ' ' << action.defender->type->plural <<
            " perish.";
    }

    logv->add(ostr.str());
}

void handleMouseMotion(const SDL_MouseMotionEvent &event)
{
    if (insideRect(event.x, event.y, bfWindow) && !logHasFocus) {
        bf->handleMouseMotion(event, getPossibleAction(event.x, event.y));
    }
}

void handleMouseDown(const SDL_MouseButtonEvent &event)
{
    if (event.button == SDL_BUTTON_LEFT &&
        insideRect(event.x, event.y, logWindow))
    {
        logv->handleMouseDown(event);
        logHasFocus = true;
    }
}

void handleMouseUp(const SDL_MouseButtonEvent &event)
{
    if (event.button == SDL_BUTTON_LEFT) {
        if (logHasFocus) {
            logv->handleMouseUp(event);
            logHasFocus = false;
        }
        else if (insideRect(event.x, event.y, bfWindow) && !gameOver) {
            auto action = getPossibleAction(event.x, event.y);
            if (action.type != ActionType::NONE) {
                action.computeDamage();
                logAction(action);
                executeAction(action);
                bf->clearHighlights();
                bf->deselectHex();
                actionTaken = true;
            }
        }
    }
}

bool parseUnits(const rapidjson::Document &doc)
{
    bool unitAdded = false;
    for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
        if (!i->value.IsObject()) {
            std::cerr << "units: skipping id '" << i->name.GetString() <<
                "'\n";
            continue;
        }

        gs->addUnitType(i->name.GetString(), UnitType(i->value));
        unitAdded = true;
    }

    return unitAdded;
}

// Create a drawable entity for the size of a unit.  Return its id.
int createUnitLabel(int num, Point hex)
{
    auto txt = sdlPreRender(labelFont, num, WHITE);
    auto id = gs->addEntity(std::move(hex), txt, ZOrder::CREATURE);
    auto &label = gs->getEntity(id);
    label.font = labelFont;
    label.alignBottomCenter();
    return id;
}

void parseScenario(const rapidjson::Document &doc)
{
    for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
        if (!i->value.IsObject()) {
            std::cerr << "scenario: skipping unit at position '"
                << i->name.GetString() << "'\n";
            continue;
        }

        // Compute battlefield position from location id.
        std::string posStr = i->name.GetString();
        auto posIter = mapUnitPos.find(posStr);
        if (posIter == std::end(mapUnitPos)) {
            std::cerr << "scenario: skipping unit at position '"
                << i->name.GetString() << "'\n";
            continue;
        }
        int posIdx = posIter->second;
        const auto &bfHex = unitPos[posIdx];

        const auto &json = i->value;

        // Ensure we recognize the unit id.
        std::string name;
        if (json.HasMember("id")) {
            name = json["id"].GetString();
        }
        const auto unitType = gs->getUnitType(name);
        if (!unitType) {
            std::cerr << "scenario: skipping unit with unknown id '" <<
                unitType << "'\n";
            continue;
        }

        Unit newUnit(*unitType);
        newUnit.team = (posIdx < 7) ? 0 : 1;
        newUnit.aHex = bf->aryFromHex(bfHex);
        newUnit.face = (newUnit.team == 0) ? Facing::RIGHT : Facing::LEFT;
        if (json.HasMember("num")) {
            newUnit.num = json["num"].GetInt();
            newUnit.labelId = createUnitLabel(newUnit.num, bfHex);
        }

        SdlSurface img;
        if (newUnit.team == 0) {
            img = newUnit.type->baseImg[0];
        }
        else {
            img = newUnit.type->reverseImg[1];
        }

        newUnit.entityId = gs->addEntity(bfHex, img, ZOrder::CREATURE);
        gs->addUnit(newUnit);
    }
}

bool checkNewRound()
{
    int nextRound = gs->getRound();
    if (nextRound == roundNum) {
        return false;
    }

    roundNum = nextRound;
    std::string msg("Round ");
    msg += boost::lexical_cast<std::string>(roundNum);
    msg += " begins.";
    logv->addBlankLine();
    logv->add(msg);
    return true;
}

extern "C" int SDL_main(int, char **)  // 2-arg form is required by SDL
{
    if (!sdlInit(288, 420, "icon.png", "Battle Sim")) {
        return EXIT_FAILURE;
    }

    gs = make_unique<GameState>();
    bf = make_unique<Battlefield>(bfWindow);

    auto font = sdlLoadFont("../DejaVuSans.ttf", 12);
    labelFont = sdlLoadFont("../DejaVuSans.ttf", 9);
    logv = make_unique<LogView>(logWindow, font);

    rapidjson::Document unitsDoc;
    if (!parseJson("units.json", unitsDoc)) {
        return EXIT_FAILURE;
    }
    if (!parseUnits(unitsDoc)) {
        std::cerr << "Error: no unit definitions loaded" << std::endl;
        return EXIT_FAILURE;
    }

    translateUnitPos();

    rapidjson::Document scenario;
    if (!parseJson("scenario.json", scenario)) {
        return EXIT_FAILURE;
    }
    parseScenario(scenario);

    gs->nextTurn();
    bf->selectHex(gs->getActiveUnit()->aHex);
    bf->draw();
    logv->add("Round 1 begins.");
    logv->draw();

    SDL_Surface *screen = SDL_GetVideoSurface();
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    bool isDone = false;
    bool needRedraw = false;
    SDL_Event event;
    while (!isDone) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isDone = true;
            }

            // Ignore mouse events while animating.
            if (!anims.empty()) continue;

            if (event.type == SDL_MOUSEMOTION && !gameOver) {
                handleMouseMotion(event.motion);
                needRedraw = true;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                handleMouseDown(event.button);
            }
            else if (event.type == SDL_MOUSEBUTTONUP) {
                handleMouseUp(event.button);
            }
        }

        // Run the current animation.
        if (!anims.empty()) {
            anims.front()->execute();
            if (anims.front()->isDone()) {
                anims.pop_front();
            }
            needRedraw = true;
        }

        // If we're finished running animations for the previous action, start
        // the next turn.
        if (actionTaken && anims.empty()) {
            actionTaken = false;
            auto winningTeam = gs->getWinner();
            if (winningTeam == Winner::NOBODY_YET) {
                gs->nextTurn();
                checkNewRound();
                bf->selectHex(gs->getActiveUnit()->aHex);
            }
            else {
                logv->addBlankLine();
                if (winningTeam == Winner::DRAW) {
                    logv->add("It's a draw.");
                }
                else {
                    auto teamNum = static_cast<int>(winningTeam);
                    std::string msg("** Team ");
                    msg += boost::lexical_cast<std::string>(teamNum);
                    msg += " wins! **";
                    logv->add(msg);
                }

                gameOver = true;
                bf->clearHighlights();
                needRedraw = true;
            }
        }

        if (needRedraw) {
            bf->draw();
            logv->draw();
            SDL_UpdateRect(screen, 0, 0, 0, 0);
        }
        SDL_Delay(1);
    }

    // Ensure SDL resources are cleaned up before the subsystems are torn down.
    gs.reset();
    bf.reset();
    labelFont.reset();
    return EXIT_SUCCESS;
}
