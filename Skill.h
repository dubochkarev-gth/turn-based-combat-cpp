// ---Skill.h ---
#pragma once
#include <string>
#include "CombatTypes.h"

enum class ResourceType
{
    None,
    Mana,
    Momentum,
    Guard
};

struct Skill
{
    std::string name;
    ActionType type;

    ResourceType resource = ResourceType::None;
    int cost = 0;

    float powerMultiplier = 1.0f;
};