// --- Item.h ---
#pragma once
#include <string>
#include "Skill.h"
#include <vector>

enum class ItemType
{
    Heal,
    Equipment
};

struct Item
{
    std::string name;
    ItemType type;
    std::vector<Skill> skills;

    int power = 0;

    float damageMultiplier = 1.0f;
    float threatMultiplier = 1.0f;
    float blockMultiplierFromEquip = 1.0f;

};