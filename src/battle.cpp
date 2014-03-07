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
#include "Action.h"
#include "Anim.h"
#include "Battlefield.h"
#include "CommanderView.h"
#include "GameState.h"
#include "HexGrid.h"
#include "LogView.h"
#include "Unit.h"
#include "UnitType.h"
#include "ai.h"
#include "algo.h"
#include "hex_utils.h"
#include "sdl_helper.h"

#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/thread/future.hpp"

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

    enum class AiState {IDLE, RUNNING, COMPLETE};
    AiState aiState = AiState::IDLE;
    boost::future<Action> aiAction;

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

    void initBattleGrid()
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
    auto aTgt = bf->aryFromPixel(px, py);
    if (aTgt < 0) {
        return {};
    }
    auto hTgt = grid->hexFromAry(aTgt);

    const auto &attacker = gs->getActiveUnit();
    if (!attacker.isAlive()) {
        return {};
    }
    const auto &defender = gs->getUnitAt(aTgt);

    if (defender.isAlive() && attacker.isEnemy(defender)) {
        auto pOffset = Point{px, py} - bf->sPixelFromHex(hTgt);
        auto attackDir = getSector(pOffset.x, pOffset.y);
        auto hMoveTo = adjacent(hTgt, attackDir);
        auto aMoveTo = grid->aryFromHex(hMoveTo);
        return gs->makeAttack(attacker.entityId, defender.entityId, aMoveTo);
    }
    else if (!defender.isAlive()) {
        return gs->makeMove(attacker.entityId, aTgt);
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

void animateAction(const Action &action)
{
    if (action.type == ActionType::NONE) {
        return;
    }

    auto &unit = gs->getUnit(action.attacker);
    assert(unit.isValid());

    // All other actions might involve moving.
    if (action.path.size() > 1) {
        auto facing = unit.face;
        for (auto i = 1u; i < action.path.size(); ++i) {
            auto hSrc = grid->hexFromAry(action.path[i - 1]);
            auto hDest = grid->hexFromAry(action.path[i]);
            auto prevFacing = facing;
            facing = getFacing(hSrc, hDest, prevFacing);

            anims.emplace_back(make_unique<AnimMove>(unit, hSrc, hDest, facing));
        }
        unit.face = facing;
    }

    // These animations happen after the attacker is done moving.
    if (action.type == ActionType::RANGED) {
        auto rangedSeq = make_unique<AnimParallel>();
        auto &defender = gs->getUnit(action.defender);
        assert(defender.isValid());

        auto hSrc = grid->hexFromAry(unit.aHex);
        auto hTgt = grid->hexFromAry(defender.aHex);

        unit.face = getFacing(hSrc, hTgt, unit.face);
        auto animShooter = make_unique<AnimRanged>(unit, hTgt);

        auto animShot = make_unique<AnimProjectile>(unit.type->projectile,
             hSrc, hTgt, animShooter->getShotTime());

        defender.face = getFacing(hTgt, hSrc, defender.face);
        auto hitTime = animShooter->getShotTime() + animShot->getFlightTime();

        rangedSeq->add(std::move(animShooter));
        rangedSeq->add(std::move(animShot));
        if (defender.isAlive()) {
            rangedSeq->add(make_unique<AnimDefend>(defender, hSrc, hitTime));
        }
        else {
            rangedSeq->add(make_unique<AnimDie>(defender, hitTime));
        }
        anims.emplace_back(std::move(rangedSeq));
    }
    else if (action.type == ActionType::ATTACK ||
             action.type == ActionType::RETALIATE)
    {
        auto attackSeq = make_unique<AnimParallel>();
        auto &defender = gs->getUnit(action.defender);
        assert(defender.isValid());

        auto hSrc = grid->hexFromAry(unit.aHex);
        auto hTgt = grid->hexFromAry(defender.aHex);

        unit.face = getFacing(hSrc, hTgt, unit.face);
        auto anim1 = make_unique<AnimAttack>(unit, hTgt);
        auto hitTime = anim1->getHitTime();

        defender.face = getFacing(hTgt, hSrc, defender.face);

        attackSeq->add(std::move(anim1));
        if (defender.isAlive()) {
            attackSeq->add(make_unique<AnimDefend>(defender, hSrc, hitTime));
        }
        else {
            attackSeq->add(make_unique<AnimDie>(defender, hitTime));
        }
        anims.emplace_back(std::move(attackSeq));
    }
    else if (action.type == ActionType::REGENERATE) {
        auto hex = grid->hexFromAry(unit.aHex);
        anims.emplace_back(make_unique<AnimRegenerate>(hex));
    }
    else if (action.type == ActionType::EFFECT) {
        const auto &target = gs->getUnit(action.defender);
        assert(target.isValid());
        auto hex = grid->hexFromAry(target.aHex);
        anims.emplace_back(make_unique<AnimEffect>(action.effect, hex));
    }
}

void logAction(const Action &action)
{
    if (action.type != ActionType::RETALIATE &&
        action.type != ActionType::REGENERATE)
    {
        std::cout << "    ";
        gs->printAction(std::cout, action);
        std::cout << std::endl;
    }

    if (action.type == ActionType::MOVE) {
        return;
    }

    std::ostringstream ostr("- ", std::ios::ate);

    if (action.type == ActionType::ATTACK ||
        action.type == ActionType::RANGED ||
        action.type == ActionType::RETALIATE ||
        action.type == ActionType::REGENERATE ||
        action.type == ActionType::NONE)
    {
        const auto &attacker = gs->getUnit(action.attacker);
        assert(attacker.isValid());
        ostr << attacker.getName();

        if (action.type == ActionType::ATTACK ||
            action.type == ActionType::RANGED)
        {
            ostr << (attacker.num == 1 ? " attacks " : " attack ");
            ostr << gs->getUnit(action.defender).getName();
        }
        else if (action.type == ActionType::RETALIATE) {
            ostr << (attacker.num == 1 ? " retaliates" : " retaliate");
        }
        else if (action.type == ActionType::NONE) {
            ostr << (attacker.num == 1 ? " skips its " : " skip their ");
            ostr << "turn.";
        }
        else if (action.type == ActionType::REGENERATE) {
            ostr << (attacker.num == 1 ? " regenerates." : " regenerate.");
        }
    }
    else if (action.type == ActionType::EFFECT) {
        const auto &defender = gs->getUnit(action.defender);
        assert(defender.isValid());
        ostr << defender.getName();
        ostr << (defender.num == 1 ? " is " : " are ");
        ostr << action.effect.getText() << '.';
    }

    if (action.type != ActionType::NONE &&
        action.type != ActionType::REGENERATE &&
        action.type != ActionType::EFFECT)
    {
        ostr << " for " << action.damage << " damage.";
        const auto &defender = gs->getUnit(action.defender);
        int numKilled = defender.simulateDamage(action.damage);
        if (numKilled > 0) {
            ostr << "  " << defender.getName(numKilled);
            ostr << (numKilled > 1 ? " perish." : " perishes.");
        }
    }

    logv->add(ostr.str());
}

void execAnimate(Action action)
{
    action.damage = gs->computeDamage(action);
    logAction(action);
    gs->execute(action);
    animateAction(action);
    bf->clearHighlights();
    bf->deselectHex();
}

bool isHumanTurn()
{
    return gs->getActiveTeam() == 0;
    //return true;
}

void handleMouseMotion(const SDL_MouseMotionEvent &event)
{
    if (insideRect(event.x, event.y, bfWindow) &&
        !logHasFocus &&
        !gs->isGameOver() &&
        isHumanTurn())
    {
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
        else if (insideRect(event.x, event.y, bfWindow) &&
                 !gs->isGameOver() &&
                 isHumanTurn())
        {
            auto action = getPossibleAction(event.x, event.y);
            if (action.type == ActionType::NONE) return;
            gs->runActionSeq(action);
            actionTaken = true;
        }
    }
}

void handleKeyPress(const SDL_KeyboardEvent &event)
{
    if (event.keysym.sym != SDLK_s ||
        logHasFocus ||
        gs->isGameOver() ||
        !isHumanTurn())
    {
        return;
    }

    const auto &unit = gs->getActiveUnit();
    if (!unit.isAlive()) return;

    auto skipAction = gs->makeSkip(unit.entityId);
    gs->runActionSeq(skipAction);
    actionTaken = true;
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
            if (posStr == "cmdr1") {
                gs->setCommander(Commander(i->value), 0);
            }
            else if (posStr == "cmdr2") {
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
    aiState = AiState::IDLE;
    gs->nextTurn();
    auto score = gs->getScore();
    checkWinner(score[0], score[1]);

    if (!gs->isGameOver()) {
        std::cout << gs->getActiveUnit().getName();
    }
    else {
        std::cout << "Game over";
    }

    std::cout << " (score: " << score[0] << '-' << score[1] << ')' << std::endl;
}

Action callNaiveAI()
{
    return aiNaive(*gs);
}

Action callBestAI()
{
    return aiBest(*gs);
}

void runAiTurn()
{
    if (aiState == AiState::IDLE) {
        if (gs->getActiveTeam() == 0) {
            aiAction = boost::async(callNaiveAI);
        }
        else {
            aiAction = boost::async(callBestAI);
        }
        aiState = AiState::RUNNING;
    }

    if (aiState == AiState::RUNNING && aiAction.is_ready()) {
        assert(aiAction.has_value());
        Action a = aiAction.get();
        gs->runActionSeq(a);
        actionTaken = true;
        aiState = AiState::COMPLETE;
    }
}

extern "C" int SDL_main(int argc, char *argv[])
{
    if (!sdlInit(winWidth, winHeight, "icon.png", "Battle Sim")) {
        return EXIT_FAILURE;
    }

    initBattleGrid();
    initEffectCache();
    bf = make_unique<Battlefield>(bfWindow, *grid);
    Anim::setBattlefield(*bf);

    gs = make_unique<GameState>(*grid);
    gs->setExecFunc(execAnimate);

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
    bf->selectHex(gs->getActiveUnit().aHex);
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

            if (event.type == SDL_MOUSEMOTION) {
                handleMouseMotion(event.motion);
                needRedraw = true;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                handleMouseDown(event.button);
            }
            else if (event.type == SDL_MOUSEBUTTONUP) {
                handleMouseUp(event.button);
            }
            else if (event.type == SDL_KEYUP) {
                handleKeyPress(event.key);
            }
        }

        if (!gs->isGameOver() && !isHumanTurn()) {
            runAiTurn();
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
            if (!gs->isGameOver()) {
                checkNewRound();
                bf->selectHex(gs->getActiveUnit().aHex);
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
