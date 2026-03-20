// --- Item.h ---
#pragma once
#include <string>

enum class ItemType
{
    Heal,
    Equipment
};

struct Item
{
    std::string name;
    ItemType type;

    int power = 0;

    float damageMultiplier = 1.0f;
    float threatMultiplier = 1.0f;
    float blockMultiplierFromEquip = 1.0f;

    bool grantsTaunt = false;
    bool grantsHeal = false;
};