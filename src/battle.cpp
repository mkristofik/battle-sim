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
#include "CommanderView.h"
#include "GameState.h"
#include "HexGrid.h"
#include "LogView.h"
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
// - leave space to write unit stats for the highlighted hex

// TODO: things we need to figure all this out
// - traits (X macros?)

namespace
{
    std::unique_ptr<HexGrid> grid;
    std::unique_ptr<GameState> gs;
    std::unique_ptr<Battlefield> bf;
    std::unique_ptr<LogView> logv;
    Uint16 winWidth = 698;
    Uint16 winHeight = 425;
    SDL_Rect cmdrWindow1 = {0, 0, 200, 230};
    SDL_Rect cmdrWindow2 = {498, 0, 200, 230};
    SDL_Rect border1 = {200, 0, 5, winHeight};
    SDL_Rect border2 = {493, 0, 5, winHeight};
    SDL_Rect border3 = {203, 360, 293, 5};
    SDL_Rect bfWindow = {205, 0, 288, 360};
    SDL_Rect logWindow = {205, 365, 288, 60};
    SdlFont labelFont;
    std::unordered_map<std::string, int> mapUnitPos;
    UnitTypeMap unitRef;
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

    void makeBattleGrid()
    {
        grid = make_unique<HexGrid>(5, 5);
        grid->erase(0, 0);
        grid->erase(0, 4);
        grid->erase(1, 4);
        grid->erase(3, 4);
        grid->erase(4, 0);
        grid->erase(4, 4);
    }

    const UnitType * getUnitType(const std::string &name)
    {
        auto iter = unitRef.find(name);
        if (iter != std::end(unitRef)) {
            return &iter->second;
        }

        return nullptr;
    }

    bool parseJson(const char *filename, rapidjson::Document &doc)
    {
        boost::filesystem::path dataPath{"../data"};
        dataPath /= filename;
        std::shared_ptr<FILE> fp{fopen(dataPath.string().c_str(), "r"), fclose};

        rapidjson::FileStream file(fp.get());
        if (doc.ParseStream<0>(file).HasParseError()) {
            std::cerr << "Error reading json file " << dataPath.string() << ": "
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

// Human player's function - determine what action the active unit can take if
// the user clicks at the given screen coordinates.
Action getPossibleAction(int px, int py)
{
    Action action;
    auto aTgt = bf->aryFromPixel(px, py);
    if (aTgt < 0) {
        return {};
    }
    auto hTgt = grid->hexFromAry(aTgt);

    action.attacker = gs->getActiveUnit();
    if (!action.attacker) {
        return {};
    }

    action.defender = gs->getUnitAt(aTgt);

    // Pathfinder includes the current hex.
    auto moveRange = static_cast<unsigned>(action.attacker->type->moves) + 1;

    if (action.defender &&
        action.defender->team == action.attacker->getEnemyTeam())
    {
        if (gs->isRangedAttackAllowed(*action.attacker)) {
            action.type = ActionType::RANGED;
            return action;
        }

        action.type = ActionType::ATTACK;
        auto pOffset = Point{px, py} - bf->sPixelFromHex(hTgt);
        auto attackDir = getSector(pOffset.x, pOffset.y);
        auto hMoveTo = adjacent(hTgt, attackDir);
        auto aMoveTo = grid->aryFromHex(hMoveTo);
        action.path = gs->getPath(action.attacker->aHex, aMoveTo);
        if (!action.path.empty() && action.path.size() <= moveRange) {
            return action;
        }
    }
    else if (!action.defender) {
        action.type = ActionType::MOVE;
        action.path = gs->getPath(action.attacker->aHex, aTgt);
        if (action.path.size() > 1 && action.path.size() <= moveRange) {
            return action;
        }
    }

    return {};
}

/*
 * TODO:
 * getAllPossibleActions():
 *      BFS to find all reachable hexes
 *      - grid->aryNeighbors lists all adjacent hexes
 *      - gs needs a more efficient getUnitAt(), possibly encapsulate unit move
 *      possible: move to each of those hexes
 *      list all adjacent enemies at each hex
 *      - gs needs function to do this
 *      - gs->isRangedAttackAllowed() can use it
 *      possible: melee attack each enemy from each hex
 *      if ranged attack allowed from current hex:
 *              possible: ranged attack each enemy
 *      possible: skip turn
 */

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

void executeAction(const Action &action)
{
    if (action.type == ActionType::NONE) {
        return;
    }

    auto unit = action.attacker;
    if (!unit) {
        return;
    }

    // All other actions might involve moving.
    if (action.path.size() > 1) {
        auto facing = unit->face;
        for (auto i = 1u; i < action.path.size(); ++i) {
            auto hSrc = grid->hexFromAry(action.path[i - 1]);
            auto hDest = grid->hexFromAry(action.path[i]);
            auto prevFacing = facing;
            facing = getFacing(hSrc, hDest, prevFacing);

            anims.emplace_back(make_unique<AnimMove>(*unit, hDest, facing));
        }
        gs->moveUnit(*unit, action.path.back());
        unit->face = facing;
    }

    // These animations happen after the attacker is done moving.
    if (action.type == ActionType::RANGED) {
        auto rangedSeq = make_unique<AnimParallel>();
        auto defender = action.defender;

        auto hSrc = grid->hexFromAry(unit->aHex);
        auto hTgt = grid->hexFromAry(defender->aHex);

        unit->face = getFacing(hSrc, hTgt, unit->face);
        auto animShooter = make_unique<AnimRanged>(*unit, hTgt);

        auto animShot = make_unique<AnimProjectile>(unit->type->projectile,
             hSrc, hTgt, animShooter->getShotTime());

        defender->face = getFacing(hTgt, hSrc, defender->face);
        defender->takeDamage(action.damage);
        auto hitTime = animShooter->getShotTime() + animShot->getFlightTime();

        rangedSeq->add(std::move(animShooter));
        rangedSeq->add(std::move(animShot));
        if (defender->isAlive()) {
            rangedSeq->add(make_unique<AnimDefend>(*defender, hSrc, hitTime));
        }
        else {
            rangedSeq->add(make_unique<AnimDie>(*defender, hitTime));
        }
        anims.emplace_back(std::move(rangedSeq));
    }
    else if (action.type == ActionType::ATTACK ||
             action.type == ActionType::RETALIATE)
    {
        auto attackSeq = make_unique<AnimParallel>();
        auto defender = action.defender;

        auto hSrc = grid->hexFromAry(unit->aHex);
        auto hTgt = grid->hexFromAry(defender->aHex);

        unit->face = getFacing(hSrc, hTgt, unit->face);
        auto anim1 = make_unique<AnimAttack>(*unit, hTgt);
        auto hitTime = anim1->getHitTime();

        defender->face = getFacing(hTgt, hSrc, defender->face);
        defender->takeDamage(action.damage);

        attackSeq->add(std::move(anim1));
        if (defender->isAlive()) {
            attackSeq->add(make_unique<AnimDefend>(*defender, hSrc, hitTime));
        }
        else {
            attackSeq->add(make_unique<AnimDie>(*defender, hitTime));
        }
        anims.emplace_back(std::move(attackSeq));
    }

    if (action.type == ActionType::RETALIATE) {
        unit->retaliated = true;
    }
}

void logAction(const Action &action)
{
    if (action.type == ActionType::MOVE || action.type == ActionType::NONE) {
        return;
    }
    if (!action.attacker || !action.defender) {
        return;
    }

    int numKilled = action.defender->simulateDamage(action.damage);

    std::ostringstream ostr("- ", std::ios::ate);
    ostr << action.attacker->getName();
    if (action.type == ActionType::ATTACK || action.type == ActionType::RANGED) {
        ostr << (action.attacker->num == 1 ? " attacks " : " attack ");
        ostr << action.defender->getName();
    }
    else if (action.type == ActionType::RETALIATE) {
        ostr << (action.attacker->num == 1 ? " retaliates" : " retaliate");
    }
    ostr << " for " << action.damage << " damage.";
    if (numKilled > 0) {
        ostr << "  " << action.defender->getName(numKilled);
        ostr << (numKilled > 1 ? " perish." : " perishes.");
    }

    logv->add(ostr.str());
}

void doAction(Action &action)
{
    gs->computeDamage(action);
    logAction(action);
    executeAction(action);
}

bool isRetaliationAllowed(const Action &action)
{
    return action.type == ActionType::ATTACK &&
           action.attacker && action.attacker->isAlive() &&
           action.defender && action.defender->isAlive() &&
           !action.defender->retaliated;
}

Action retaliate(const Action &action)
{
    Action retaliation;
    retaliation.attacker = action.defender;
    retaliation.defender = action.attacker;
    retaliation.type = ActionType::RETALIATE;
    return retaliation;
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
                doAction(action);
                if (isRetaliationAllowed(action)) {
                    auto retaliation = retaliate(action);
                    doAction(retaliation);
                }
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

        unitRef.emplace(i->name.GetString(), UnitType(i->value));
        unitAdded = true;
    }

    return unitAdded;
}

// Create a drawable entity for the size of a unit.  Return its id.
int createUnitLabel(int num, int team, Point hex)
{
    auto txt = sdlPreRender(labelFont, num, getLabelColor(team));
    auto id = bf->addEntity(std::move(hex), txt, ZOrder::CREATURE);
    auto &label = bf->getEntity(id);
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
            if (posStr == "c1") {
                gs->setCommander(Commander(i->value), 0);
            }
            else if (posStr == "c2") {
                gs->setCommander(Commander(i->value), 1);
            }
            else {
                std::cerr << "scenario: skipping unit at position '"
                    << i->name.GetString() << "'\n";
            }
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
        const auto unitType = getUnitType(name);
        if (!unitType) {
            std::cerr << "scenario: skipping unit with unknown id '" <<
                unitType << "'\n";
            continue;
        }

        Unit newUnit(*unitType);
        newUnit.team = (posIdx < 7) ? 0 : 1;
        newUnit.aHex = grid->aryFromHex(bfHex);
        newUnit.face = (newUnit.team == 0) ? Facing::RIGHT : Facing::LEFT;
        if (json.HasMember("num")) {
            newUnit.num = json["num"].GetInt();
            newUnit.labelId = createUnitLabel(newUnit.num, newUnit.team, bfHex);
        }

        SdlSurface img;
        if (newUnit.team == 0) {
            img = newUnit.type->baseImg[0];
        }
        else {
            img = newUnit.type->reverseImg[1];
        }

        newUnit.entityId = bf->addEntity(bfHex, img, ZOrder::CREATURE);
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

const char * getScenario(int argc, char *argv[])
{
    if (argc < 2) {
        return "scenario.json";
    }
    return argv[1];
}

SDL_Rect getBorderFgLine(const SDL_Rect &border)
{
    auto line = border;
    if (border.h > border.w) {
        line.x += border.w / 2;
        line.w = 1;
    }
    else {
        line.y += border.h / 2;
        line.h = 1;
    }
    return line;
}

void drawBorders()
{
    auto screen = SDL_GetVideoSurface();
    auto bgColor = SDL_MapRGB(screen->format, BORDER_BG.r, BORDER_BG.g,
                               BORDER_BG.b);
    auto fgColor = SDL_MapRGB(screen->format, BORDER_FG.r, BORDER_FG.g,
                               BORDER_FG.b);

    SDL_FillRect(screen, &border1, bgColor);
    SDL_FillRect(screen, &border2, bgColor);
    SDL_FillRect(screen, &border3, bgColor);

    SDL_Rect line1 = getBorderFgLine(border1);
    SDL_FillRect(screen, &line1, fgColor);
    SDL_Rect line2 = getBorderFgLine(border2);
    SDL_FillRect(screen, &line2, fgColor);
    SDL_Rect line3 = getBorderFgLine(border3);
    SDL_FillRect(screen, &line3, fgColor);
}

bool checkWinner(int score1, int score2)
{
    if (score1 == 0 && score2 == 0) {
        logv->addBlankLine();
        logv->add("It's a draw.");
        return true;
    }
    else if (score1 == 0) {
        logv->addBlankLine();
        logv->add("** Team 2 wins! **");
        return true;
    }
    else if (score2 == 0) {
        logv->addBlankLine();
        logv->add("** Team 1 wins! **");
        return true;
    }

    return false;
}

void nextTurn()
{
    auto score = gs->getScore();
    gameOver = checkWinner(score.first, score.second);

    if (!gameOver) {
        gs->nextTurn();
        auto unit = gs->getActiveUnit();
        std::cout << unit->getName();
    }
    else {
        std::cout << "Game over";
    }
    std::cout << " (score: " << score.first << '-' << score.second << ")\n";
}

extern "C" int SDL_main(int argc, char *argv[])
{
    if (!sdlInit(winWidth, winHeight, "icon.png", "Battle Sim")) {
        return EXIT_FAILURE;
    }

    makeBattleGrid();
    bf = make_unique<Battlefield>(bfWindow, *grid);
    gs = make_unique<GameState>(*grid);
    Anim::setBattlefield(*bf);

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
    if (!parseJson(getScenario(argc, argv), scenario)) {
        return EXIT_FAILURE;
    }
    parseScenario(scenario);

    CommanderView cView1{gs->getCommander(0), 0, font, cmdrWindow1};
    CommanderView cView2{gs->getCommander(1), 1, font, cmdrWindow2};

    nextTurn();
    bf->selectHex(gs->getActiveUnit()->aHex);
    bf->draw();
    logv->add("Round 1 begins.");
    logv->draw();
    cView1.draw();
    cView2.draw();
    drawBorders();

    auto screen = SDL_GetVideoSurface();
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
            nextTurn();
            if (!gameOver) {
                checkNewRound();
                bf->selectHex(gs->getActiveUnit()->aHex);
            }
            else {
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
    // TODO: can we encapsulate these so this isn't necessary?
    gs.reset();
    bf.reset();
    labelFont.reset();
    return EXIT_SUCCESS;
}
