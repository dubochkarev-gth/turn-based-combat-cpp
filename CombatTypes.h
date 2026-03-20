// --- CombatTypes.h ---
#pragma once
#include <string>

// CONSTANTS

constexpr float FOCUS_BONUS_MULTIPLIER = 1.5f;
constexpr int CRIT_CHANCE_PERCENT = 20;
constexpr float CRIT_MULTIPLIER = 2.0f;
constexpr float BLOCK_BONUS_MULTIPLIER = 0.65f;

enum class ActionType
{
    Attack,
    UseItem,
    Block,
    Taunt,
    Burst,
    Heal
};

struct ActionResult
{
    ActionType type;
    std::string actor;
    std::string target;

    std::string itemName;
    int damagePlanned = 0;
    int damageApplied = 0;
    int damageBlocked = 0;
    int healedPlanned = 0;
    bool isCritical = false;
    bool targetDied = false;
    bool usedFocus = false;
    bool cancelled = false;
};