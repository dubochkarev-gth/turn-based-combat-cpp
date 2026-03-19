// --- main.cpp ---
#include <iostream>
#include <string>
#include <random>
#include <algorithm>
#include <vector>
#include <limits>
#include <unordered_map>
#include <iomanip>
#include "Entity.h"
#include "CombatTypes.h"
#include "Inventory.h"
#include "ItemSystem.h"
#include "SkillSystem.h"
#include "Simulation.h"

using namespace std;

int randomInt(int min, int max)
{
    static mt19937 gen(random_device{}());
    uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

// clear screen helper
void clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
};

struct FighterStats
{
    std::string name;

    int damageDealt = 0;
    int damageBlocked = 0;
    int healingDone = 0;

    int hits = 0;
    int maxHit = 0;

    int remainingHP = 0;
};

enum class BattleResult
{
    PlayerWin,
    EnemyWin
};

struct BattleSummary
{
    BattleResult result;
    int rounds = 0;
    std::vector<FighterStats> fighters;
};

// STRUCTS

enum class TargetType
{
    Self,
    FirstAliveEnemy,
    LowestHpAlly
};

struct PlannedAction
{
    Entity *actor = nullptr;
    ActionType type;
    TargetType targetType;
};

struct ResolvedAction
{
    Entity *actor = nullptr;
    ActionType type;
    Entity *target = nullptr;
};

struct BattleLog
{
    vector<ActionResult> actions;

    void clear()
    {
        actions.clear();
    }

    void add(const ActionResult &result)
    {
        actions.push_back(result);
    }
};

vector<Entity *> resolveTargets(
    const PlannedAction &action,
    const vector<Entity *> &entities);

bool validateAction(
    const PlannedAction &action,
    Entity *target,
    ActionResult &outResult);

void executeAction(
    const ResolvedAction &action,
    BattleLog &log);

void applyActionResult(
    ActionResult &result,
    Entity &actor,
    Entity &target);

void startTurn(Entity &e);
void endTurn(Entity &e);

void renderBattleScreen(
    const vector<Entity *> &entities,
    const BattleLog &log,
    const vector<Entity *> &turnOrder);

void buildTurnOrder(
    const vector<Entity *> &entities,
    vector<Entity *> &turnOrder);

vector<PlannedAction> planTurn(
    const vector<Entity *> &turnOrder, bool interactive);

class Battle
{
private:
    vector<Entity *> entities;
    BattleLog log;
    vector<Entity *> turnOrder;
    unordered_map<std::string, FighterStats> statsMap;
    int roundCounter = 0;
    bool interactive = false;

public:
    Battle(std::vector<Entity *> ents, bool interactiveMode)
        : entities(ents), interactive(interactiveMode)
    {
    }

    BattleSummary run()
    {
        BattleSummary summary;
        roundCounter = 0;
        statsMap.clear();
        log.clear();
        turnOrder.clear();

        for (Entity *e : entities)
        {
            FighterStats fs;
            fs.name = e->get_name();
            statsMap[e->get_name()] = fs;
        }

        while (true)
        {

            roundCounter++;

            buildTurnOrder(entities, turnOrder);
            log.clear();

            std::vector<PlannedAction> plannedActions =
                planTurn(turnOrder, interactive);

            for (PlannedAction &action : plannedActions)
            {

                if (!action.actor || !action.actor->is_alive())
                    continue;

                startTurn(*action.actor);

                std::vector<Entity *> targets =
                    resolveTargets(action, entities);

                for (Entity *target : targets)
                {

                    size_t before = log.actions.size();

                    ResolvedAction resolved;
                    resolved.actor = action.actor;
                    resolved.type = action.type;
                    resolved.target = target;

                    ActionResult validationResult;

                    if (!validateAction(action, target, validationResult))
                    {
                        log.add(validationResult);
                        continue;
                    }

                    executeAction(resolved, log);

                    if (log.actions.size() > before)
                    {
                        ActionResult &last = log.actions.back();
                        applyActionResult(last, *action.actor, *target);

                        statsMap[last.actor].damageDealt += last.damageApplied;
                        statsMap[last.target].damageBlocked += last.damageBlocked;
                        statsMap[last.actor].healingDone += last.healedPlanned;
                        if (last.damageApplied > 0)
                        {
                            FighterStats &fs = statsMap[last.actor];

                            fs.hits++;
                            fs.maxHit = std::max(fs.maxHit, last.damageApplied);
                        }

                        // --- Threat generation ---
                        if (last.damageApplied > 0)
                        {
                            float normalized =
                                static_cast<float>(last.damageApplied) /
                                static_cast<float>(target->get_max_hp());

                            action.actor->add_threat(normalized);
                        }

                        if (last.healedPlanned > 0)
                        {
                            float normalized =
                                static_cast<float>(last.healedPlanned) /
                                static_cast<float>(target->get_max_hp());

                            action.actor->add_threat(normalized * 0.5f);
                        }
                    }
                }

                endTurn(*action.actor);
            }

            // Threat decay after full round
            for (Entity *e : entities)
            {
                e->decay_threat(0.8f);
            }

            if (interactive)
                renderBattleScreen(entities, log, turnOrder);

            // Проверка победы
            bool playerAlive = false;
            bool enemiesAlive = false;

            for (Entity *e : entities)
            {

                if (e->getFaction() == Faction::Player && e->is_alive())
                    playerAlive = true;

                if (e->getFaction() == Faction::Enemy && e->is_alive())
                    enemiesAlive = true;
            }

            if (!playerAlive)
            {
                summary.result = BattleResult::EnemyWin;

                if (interactive)
                {
                    std::cout << "\n=== Battle Finished ===\n";
                    std::cout << "Enemies win!\n";
                }

                break;
            }

            if (!enemiesAlive)
            {
                summary.result = BattleResult::PlayerWin;

                if (interactive)
                {
                    std::cout << "\n=== Battle Finished ===\n";
                    std::cout << "Player wins!\n";
                }

                break;
            }

            if (interactive)
            {
                std::cout << "\nPress Enter to continue...";
                std::cin.ignore(numeric_limits<streamsize>::max(), '\n');
                std::cin.get();
            }
        }
        summary.rounds = roundCounter;

        for (Entity *e : entities)
        {
            statsMap[e->get_name()].remainingHP = e->get_hp();
        }

        for (auto &pair : statsMap)
        {
            summary.fighters.push_back(pair.second);
        }

        return summary;
    };
};

// SCREEN DRAW AFTER CALCULATION

void renderBattleScreen(
    const std::vector<Entity *> &entities,
    const BattleLog &log,
    const std::vector<Entity *> &turnOrder)
{
    clearScreen();

    std::cout << "====== BATTLE ======\n\n";

    for (const Entity *e : entities)
    {
        char marker = 'E';
        if (e->getFaction() == Faction::Player)
            marker = (e->has_taunt() ? 'T' : 'D');

        std::cout << "[" << marker << "] ";

        std::cout << std::left << std::setw(15)
                  << e->get_name();

        std::cout << " HP:"
                  << e->get_hp() << "/"
                  << e->get_max_hp();

        if (e->has_taunt())
        {
            std::cout << " G:" << e->get_guard();
        }
        else if (e->getFaction() == Faction::Player)
        {
            std::cout << " M:" << e->get_momentum();
        }
        else
        {
            if (e->get_focus() > 0)
                std::cout << " F:" << e->get_focus();
        }

        std::cout << " Th:"
                  << std::fixed << std::setprecision(2)
                  << e->get_threat()
                  << std::defaultfloat;

        std::cout << "\n";
    }

    std::cout << "\n--- Initiative order ---\n";

    for (const Entity *ent : turnOrder)
    {

        if (!ent->is_alive())
            continue;

        std::cout << ent->get_name() << " (" << ent->getInitiative() << ")"
                  << "-----";
    }

    std::cout << "\n";

    std::cout << "\n--- Last turn ---\n";

    for (const ActionResult &r : log.actions)
    {
        if (r.cancelled)
        {
            std::cout << r.actor << " tries to attack, but " << r.target
                      << " is already dead";
            continue;
        }
        if (r.type == ActionType::Attack)
        {
            std::cout << r.actor << " hits " << r.target
                      << " for " << r.damageApplied << ".";

            if (r.isCritical)
                std::cout << " (CRITICAL)";

            if (r.usedFocus)
                std::cout << " (FOCUSED)";
        }

        if (r.type == ActionType::Block)
        {
            std::cout << r.actor << " prepares to block";
        }

        if (r.damageBlocked > 0)
        {
            std::cout << " " << r.target << " blocks " << r.damageBlocked << " damage";
        }

        if (r.type == ActionType::UseItem && r.healedPlanned > 0)
        {
            std::cout << r.actor << " uses " << r.itemName << " and heals for "
                      << r.healedPlanned << " HP";
        }
        if (r.type == ActionType::Taunt)
        {
            std::cout << r.actor << " uses Taunt and increases threat.";
        }
        if (r.type == ActionType::Burst)
        {
            std::cout << r.actor << " hits with Bursted attack " << r.target
                      << " for " << r.damageApplied << ".";

            if (r.isCritical)
                std::cout << " (CRITICAL)";

            if (r.usedFocus)
                std::cout << " (FOCUSED)";
        }

        std::cout << std::endl;

        if (r.targetDied)
        {
            std::cout << r.target << " is defeated!" << std::endl;
        }
    }

    std::cout << "\n--------------------\n";
}

// Initiative calculator

void buildTurnOrder(const std::vector<Entity *> &entities,
                    std::vector<Entity *> &turnOrder)
{
    turnOrder = entities;

    sort(turnOrder.begin(), turnOrder.end(),
         [](Entity *a, Entity *b)
         {
             return a->getInitiative() > b->getInitiative();
         });
}

// Executor for battle

void executeAction(const ResolvedAction &action,
                   BattleLog &log)
{
    if (!action.actor || !action.actor->is_alive())
        return;

    ActionResult result;

    switch (action.type)
    {
    case ActionType::Attack:
        result = action.actor->attack(*action.target);
        break;

    case ActionType::Block:
        result = action.actor->block();
        break;

    case ActionType::UseItem:
        result = ItemSystem::useItem(*action.actor);
        break;
    case ActionType::Taunt:
        result = action.actor->taunt();
        break;
    case ActionType::Burst:
        result = SkillSystem::burst(*action.actor, *action.target);
        break;
    }

    log.add(result);
};

std::vector<Entity *> resolveTargets(
    const PlannedAction &action,
    const std::vector<Entity *> &entities)
{
    std::vector<Entity *> result;

    if (action.targetType == TargetType::Self)
    {
        result.push_back(action.actor);
    }

    if (action.targetType == TargetType::LowestHpAlly)
    {
        Entity *best = nullptr;
        int lowestHp = INT_MAX;

        for (Entity *e : entities)
        {
            if (e->getFaction() == action.actor->getFaction() && e->is_alive())
            {
                if (e->get_hp() < lowestHp)
                {
                    lowestHp = e->get_hp();
                    best = e;
                }
            }
        }

        if (best)
            result.push_back(best);
    }

    if (action.targetType == TargetType::FirstAliveEnemy)
    {
        Entity *best = nullptr;

        if (action.actor->getFaction() == Faction::Enemy)
        {
            float maxThreat = -1.0f;

            for (Entity *e : entities)
            {
                if (e->getFaction() == Faction::Player && e->is_alive())
                {
                    if (e->get_threat() > maxThreat)
                    {
                        maxThreat = e->get_threat();
                        best = e;
                    }
                }
            }
        }

        else
        {
            float maxThreat = -1.0f;

            for (Entity *e : entities)
            {
                if (e->getFaction() == Faction::Enemy && e->is_alive())
                {
                    if (e->get_threat() > maxThreat)
                    {
                        maxThreat = e->get_threat();
                        best = e;
                    }
                }
            }
        }

        if (!best)
        {
            for (Entity *e : entities)
            {
                if (e->getFaction() != action.actor->getFaction() && e->is_alive())
                {
                    best = e;
                    break;
                }
            }
        }

        if (best)
            result.push_back(best);
    }

    return result;
}

bool validateAction(
    const PlannedAction &action,
    Entity *target,
    ActionResult &outResult)
{
    outResult.actor = action.actor ? action.actor->get_name() : "unknown";
    outResult.target = target ? target->get_name() : "none";
    outResult.type = action.type;
    outResult.cancelled = true;

    // 1. Actor must be alive
    if (!action.actor || !action.actor->is_alive())
    {
        return false;
    }

    // 2. Target required for attack
    if ((action.type == ActionType::Attack || action.type == ActionType::Burst) && !target)
        return false;

    // 3. Target must be alive for attack
    if ((action.type == ActionType::Attack || action.type == ActionType::Burst) && !target->is_alive())
        return false;

    // 4. Item availability
    if (action.type == ActionType::UseItem)
    {
        Inventory *inv = action.actor->getInventory();
        if (!inv || inv->empty())
            return false;
    }

    // OK
    outResult.cancelled = false;
    return true;
}

void applyActionResult(ActionResult &result,
                       Entity &actor,
                       Entity &target)
{
    switch (result.type)
    {
    case ActionType::Attack:
    {
        int actualDamage = target.receive_damage(result.damagePlanned);
        result.damageApplied = actualDamage;
        result.damageBlocked = result.damagePlanned - actualDamage;
        result.targetDied = !target.is_alive();
        if (actor.getFaction() == Faction::Player && !actor.has_taunt())
        {
            actor.add_momentum(1);
        }
        break;
    }

    case ActionType::Block:
    {
        actor.set_blocking(true);
        actor.add_focus(1);
        if (actor.has_taunt())
        {
            actor.add_guard(1);
        }
        break;
    }

    case ActionType::UseItem:
    {
        int before = target.get_hp();
        target.heal(result.healedPlanned);
        result.healedPlanned = target.get_hp() - before;
        break;
    }
    case ActionType::Burst:
    {
        int actualDamage = target.receive_damage(result.damagePlanned);
        result.damageApplied = actualDamage;
        result.damageBlocked = result.damagePlanned - actualDamage;
        result.targetDied = !target.is_alive();
        break;
    }
    }
}

TargetType targetTypeSelection(ActionType a)
{
    if (a == ActionType::Attack || a == ActionType::Burst)
        return TargetType::FirstAliveEnemy;

    if (a == ActionType::UseItem)
        return TargetType::LowestHpAlly;

    return TargetType::Self;
}

// Decision function

std::vector<PlannedAction> planTurn(
    const std::vector<Entity *> &turnOrder,
    bool interactive)
{
    std::vector<PlannedAction> plannedActions;

    for (Entity *actor : turnOrder)
    {
        if (!actor->is_alive())
            continue;

        PlannedAction action;
        action.actor = actor;

        if (interactive &&
            actor->getFaction() == Faction::Player)
        {
            int choice = 0;

            std::cout << "\n"
                      << actor->get_name()
                      << " choose action:\n";
            std::cout << "1 - Attack\n";
            std::cout << "2 - Block\n";
            if (actor->hasItems())
                std::cout << "3 - Use Item\n";
            if (actor->has_taunt() && actor->get_guard() >= 3)
                std::cout << "4 - Taunt\n";
            if (actor->get_momentum() >= 2)
                std::cout << "5 - Burst\n";

            while (choice < 1 || choice > 5)
            {
                std::cin >> choice;
            }

            if (choice == 1)
                action.type = ActionType::Attack;
            else if (choice == 2)
                action.type = ActionType::Block;
            else if (choice == 4 && actor->has_taunt())
                action.type = ActionType::Taunt;
            else if (choice == 5)
                action.type = ActionType::Burst;
            else
                action.type = ActionType::UseItem;
        }
        else
        {
            action.type = actor->decideAction(turnOrder);
        }

        action.targetType = targetTypeSelection(action.type);
        plannedActions.push_back(action);
    }

    return plannedActions;
}

void startTurn(Entity &e)
{
    // future: status tick, regen, focus decay
    e.set_blocking(false);
};

void endTurn(Entity &e) {
    // future: status tick, regen, focus decay
};

void runSimulation(int runs)
{
    SimulationStats stats;

    for (int i = 0; i < runs; i++)
    {
        Player hero("Dark_Avanger", 100, 10, 5);
        Player hero2("Shadow_Blader", 90, 12, 4);

        Enemy striker("Rage_Striker", 55, 14, 9, 4);
        Enemy orc("Gazkul_Trakka", 90, 9, 7, 4);
        Enemy kobold("Ugly_Gobby", 55, 15, 5, 3);
        Enemy healer("Dark_Priest", 60, 11, 5, 2);

        healer.set_isHealer(true);
        auto healerInv = std::make_unique<Inventory>();
        healerInv->add({"Dark Heal", ItemType::Heal, 12});
        healerInv->add({"Dark Heal", ItemType::Heal, 12});
        healerInv->add({"Dark Heal", ItemType::Heal, 12});
        healer.attachInventory(std::move(healerInv));

        auto heroInv = std::make_unique<Inventory>();
        heroInv->add({"Small Potion", ItemType::Heal, 7});
        heroInv->add({"Small Potion", ItemType::Heal, 7});
        hero.attachInventory(std::move(heroInv));

        auto orcInv = std::make_unique<Inventory>();
        orcInv->add({"Crude Potion", ItemType::Heal, 10});
        orcInv->add({"Crude Potion", ItemType::Heal, 10});
        orc.attachInventory(std::move(orcInv));

        Item tankCore;
        tankCore.name = "Bulwark Armor";
        tankCore.type = ItemType::Equipment;
        tankCore.damageMultiplier = 0.85f;
        tankCore.threatMultiplier = 1.6f;
        tankCore.blockMultiplierFromEquip = 0.8f;

        Item dpsCore;
        dpsCore.name = "Executioner Blade";
        dpsCore.type = ItemType::Equipment;
        dpsCore.damageMultiplier = 1.4f;
        dpsCore.threatMultiplier = 0.7f;

        hero.equip(tankCore);
        hero2.equip(dpsCore);

        hero.setAutoMode(true);
        hero2.setAutoMode(true);

        std::vector<Entity *> entities =
            {&hero, &hero2, &healer, &orc, &kobold};

        Battle battle(entities, false);
        BattleSummary summary = battle.run();

        stats.runs++;
        stats.totalRounds += summary.rounds;

        if (summary.result == BattleResult::PlayerWin)
            stats.playerWins++;
        else
            stats.enemyWins++;

        for (const auto &f : summary.fighters)
        {
            stats.totalDamage[f.name] += f.damageDealt;
            stats.totalMaxHit[f.name] =
                std::max(stats.totalMaxHit[f.name], f.maxHit);
        }
    }

    std::cout << "\n===== SIMULATION RESULT =====\n";

    std::cout << "Runs: " << stats.runs << "\n";

    std::cout << "Player win rate: "
              << (100.0 * stats.playerWins / stats.runs)
              << "%\n";

    std::cout << "Enemy win rate: "
              << (100.0 * stats.enemyWins / stats.runs)
              << "%\n";

    std::cout << "Average rounds: "
              << ((double)stats.totalRounds / stats.runs)
              << "\n\n";

    std::cout << "--- Average Damage ---\n";

    for (auto &pair : stats.totalDamage)
    {
        std::cout << pair.first
                  << " avg damage: "
                  << pair.second / stats.runs
                  << " max hit observed: "
                  << stats.totalMaxHit[pair.first]
                  << "\n";
    }
}

// --------------------
// Main
// --------------------
int main()
{
    Player hero("Dark_Avanger", 100, 10, 5);
    Player hero2("Shadow_Blader", 90, 12, 4);

    Enemy striker("Rage_Striker", 55, 14, 9, 4);
    Enemy orc("Gazkul_Trakka", 90, 9, 7, 4);
    Enemy kobold("Ugly_Gobby", 55, 15, 5, 3);
    Enemy healer("Dark_Priest", 60, 11, 5, 2);

    healer.set_isHealer(true);
    auto healerInv = std::make_unique<Inventory>();
    healerInv->add({"Dark Heal", ItemType::Heal, 12});
    healerInv->add({"Dark Heal", ItemType::Heal, 12});
    healerInv->add({"Dark Heal", ItemType::Heal, 12});
    healer.attachInventory(std::move(healerInv));

    auto heroInv = std::make_unique<Inventory>();
    heroInv->add({"Small Potion", ItemType::Heal, 7});
    heroInv->add({"Small Potion", ItemType::Heal, 7});
    hero.attachInventory(std::move(heroInv));

    auto orcInv = std::make_unique<Inventory>();
    orcInv->add({"Crude Potion", ItemType::Heal, 10});
    orcInv->add({"Crude Potion", ItemType::Heal, 10});
    orc.attachInventory(std::move(orcInv));

    std::vector<Entity *> battleEntities = {&hero, &hero2, &healer, &orc, &kobold};

    Item tankCore;
    tankCore.name = "Bulwark Armor";
    tankCore.type = ItemType::Equipment;
    tankCore.damageMultiplier = 0.85f;
    tankCore.threatMultiplier = 1.6f;
    tankCore.blockMultiplierFromEquip = 0.9f;
    tankCore.grantsTaunt = true;

    Item dpsCore;
    dpsCore.name = "Executioner Blade";
    dpsCore.type = ItemType::Equipment;
    dpsCore.damageMultiplier = 1.4f;
    dpsCore.threatMultiplier = 0.7f;

    hero.equip(tankCore);
    hero2.equip(dpsCore);

    hero.setAutoMode(true);
    hero2.setAutoMode(true);

    BattleSummary summary;

    bool simflag = false;

    if (!simflag)
    {
        Battle battle(battleEntities, true);
        summary = battle.run();
    }
    else
    {
        runSimulation(10000);
        return 0;
    }

    std::cout << "\n=== Battle Summary ===\n";

    std::cout << "Winner: ";
    if (summary.result == BattleResult::PlayerWin)
        std::cout << "Player\n";
    else
        std::cout << "Enemies\n";
    std::cout << "Rounds: " << summary.rounds << "\n";

    for (const auto &f : summary.fighters)
    {
        int avgHit = 0;

        if (f.hits > 0)
            avgHit = f.damageDealt / f.hits;

        std::cout << f.name
                  << " | HP: " << f.remainingHP
                  << " | Dealt: " << f.damageDealt
                  << " | MaxHit: " << f.maxHit
                  << " | AvgHit: " << avgHit
                  << " | Blocked: " << f.damageBlocked
                  << " | Healed: " << f.healingDone
                  << std::endl;
    }

    return 0;
}