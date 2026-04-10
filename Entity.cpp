// --- Entity.cpp ---
#include "Entity.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "Inventory.h"

extern int randomInt(int min, int max);

// =======================
// Entity
// =======================

Entity::Entity(std::string n, int h, int baseInitiative, Faction f)
    : hp(h), max_hp(h), name(n), faction(f)
{
    stats.baseInitiative = baseInitiative;
}

void Entity::info() const
{
    std::cout << name
              << " | HP: " << hp;

    if (focus > 0)
        std::cout << " | Focus: " << focus;

    std::cout << " | Init: " << stats.baseInitiative
              << " | Threat: "
              << std::fixed << std::setprecision(2)
              << threat
              << std::defaultfloat
              << std::endl;
}

int Entity::get_hp() const { return hp; }
int Entity::get_max_hp() const { return max_hp; }
int Entity::get_focus() const { return focus; }
std::string Entity::get_name() const { return name; }
Faction Entity::getFaction() const { return faction; }
bool Entity::is_alive() const { return hp > 0; }

void Entity::heal(int amount)
{
    hp = std::min(hp + amount, max_hp);
}

int Entity::receive_damage(int amount)
{
    if (isBlocking)
    {
        amount = static_cast<int>(
            amount * BLOCK_BONUS_MULTIPLIER * blockMultiplierFromEquip);
    }

    hp -= amount;
    if (hp < 0)
        hp = 0;

    return amount;
}

void Entity::set_blocking(bool block)
{
    isBlocking = block;
}

void Entity::add_focus(int amount)
{
    focus += amount;
}

void Entity::consume_focus()
{
    focus = 0;
}

bool Entity::has_focus() const
{
    return focus > 0;
}

int Entity::get_attack_power() const
{
    return 10;
}

int Entity::getInitiative() const
{
    return stats.baseInitiative;
}

int Entity::get_guard() const
{
    return resources.guard;
}

void Entity::add_guard(int amount)
{
    resources.guard += amount;
}

bool Entity::spend_guard(int amount)
{
    if (resources.guard < amount)
        return false;

    resources.guard -= amount;
    return true;
}

int Entity::get_momentum() const
{
    return resources.momentum;
}

void Entity::add_momentum(int amount)
{
    resources.momentum += amount;

    if (resources.momentum > 6)
        resources.momentum = 6;
}

bool Entity::spend_momentum(int amount)
{
    if (resources.momentum < amount)
        return false;

    resources.momentum -= amount;
    return true;
}

void Entity::reset_momentum()
{
    resources.momentum = 0;
}

// =======================
// Combat logic
// =======================

ActionResult Entity::attack(Entity &target)
{
    ActionResult result;

    result.type = ActionType::Attack;
    result.actor = name;
    result.target = target.get_name();

    int dmg = randomInt(1, get_attack_power());

    bool crit = randomInt(1, 100) <= CRIT_CHANCE_PERCENT;
    if (crit)
    {
        dmg = static_cast<int>(dmg * CRIT_MULTIPLIER);
        result.isCritical = true;
    }

    if (has_focus())
    {
        dmg = static_cast<int>(dmg * FOCUS_BONUS_MULTIPLIER);
        consume_focus();
        result.usedFocus = true;
    }

    dmg = static_cast<int>(dmg * damageMultiplier);
    result.damagePlanned = dmg;
    return result;
}

ActionResult Entity::block()
{
    ActionResult result;
    result.type = ActionType::Block;
    result.actor = name;
    result.target = name;
    return result;
}

ActionType Entity::decideAction(const std::vector<Entity *> &entities)
{
    return ActionType::Block;
}

void Entity::add_threat(float amount)
{
    threat += amount * threatMultiplier;
}

float Entity::get_threat() const
{
    return threat;
}

void Entity::decay_threat(float factor)
{
    threat *= factor;
}

void Entity::reset_threat()
{
    threat = 0.0f;
}

// Item logic

bool Entity::hasItems() const
{
    return inventory && !inventory->empty();
}

void Entity::equip(const Item &item)
{
    if (item.type != ItemType::Equipment)
        return;

    damageMultiplier *= item.damageMultiplier;
    threatMultiplier *= item.threatMultiplier;
    blockMultiplierFromEquip *= item.blockMultiplierFromEquip;

    for (const Skill &s : item.skills)
    {
        skills.push_back(s);

        if (s.resource == ResourceType::Mana)
            add_mana(2);

        if (s.resource == ResourceType::Guard)
            add_guard(1);
    }
}

// =======================
// Player
// =======================

Player::Player(std::string name, int hp, int baseInitiative, int weapon)
    : Entity(name, hp, baseInitiative, Faction::Player),
      weapon_bonus(weapon)
{
}

int Player::get_attack_power() const
{
    return 10 + weapon_bonus;
}

void Player::setAutoMode(bool value)
{
    autoMode = value;
}

void Player::info() const
{
    std::cout << name
              << " | HP: " << hp;

    if (focus > 0)
        std::cout << " | Focus: " << focus;

    std::cout << " | Init: " << stats.baseInitiative
              << " | Threat: "
              << std::fixed << std::setprecision(2)
              << threat
              << std::defaultfloat
              << std::endl;
}

ActionType Player::decideAction(const std::vector<Entity *> &entities)
{
    if (autoMode)
    {
        if (get_hp() < get_max_hp() * 50 / 100 && hasItems())
            return ActionType::UseItem;

        if (has_focus())
            return ActionType::Attack;

        for (const Skill &s : getSkills())
        {
            if (s.type == ActionType::Heal && get_mana() >= s.cost)
            {
                for (auto *e : entities)
                {
                    if (e->getFaction() == getFaction() &&
                        e->is_alive() &&
                        e->get_hp() < e->get_max_hp() * 0.6)
                    {
                        return ActionType::Heal;
                    }
                }
            }
        }

        int roll = randomInt(1, 100);
        return (roll <= 40) ? ActionType::Attack : ActionType::Block;
    }

    int playerChoice = 0;

    while (playerChoice < 1 || playerChoice > 3)
    {
        std::cout << "Player make a choice:\n";
        std::cout << "1 - Attack\n2 - Block\n3 - Use Item\n";
        std::cin >> playerChoice;
    }

    if (playerChoice == 1)
        return ActionType::Attack;

    if (playerChoice == 2)
        return ActionType::Block;

    if (playerChoice == 3)
    {
        if (!hasItems())
        {
            std::cout << "No items left!\n";
            return ActionType::Block;
        }
        return ActionType::UseItem;
    }

    return ActionType::Block;
}

// =======================
// Enemy
// =======================

Enemy::Enemy(std::string name, int hp, int baseInitiative, int baseAtk, int str)
    : Entity(name, hp, baseInitiative, Faction::Enemy),
      base_attack(baseAtk),
      strength(str)
{
}

int Enemy::get_attack_power() const
{
    return base_attack + strength;
}

void EnemyAI::update(int hp, bool canHealNow, bool hasFocusNow, bool curAllyLowHp)
{
    canHeal = canHealNow;
    hasFocus = hasFocusNow;
    allyLowHp = curAllyLowHp;
    if (hp < 20)
        state = AIState::Desperate;
    else if (hp < 40)
        state = AIState::Defensive;
    else
        state = AIState::Aggressive;
}

ActionType EnemyAI::decideAction() const
{
    if (canHeal && allyLowHp)
        return ActionType::UseItem;

    switch (state)
    {
    case AIState::Aggressive:
        return ActionType::Attack;

    case AIState::Defensive:
    {
        int roll = randomInt(0, 1);
        if (roll == 0)
            return ActionType::Attack;
        if (canHeal)
            return ActionType::UseItem;
        return ActionType::Block;
    }

    case AIState::Desperate:
    {
        if (canHeal)
            return ActionType::UseItem;

        return hasFocus ? ActionType::Attack : ActionType::Block;
    }
    }

    return ActionType::Attack;
}

ActionType Enemy::decideAction(const std::vector<Entity *> &entities)
{
    bool allyLowHp = false;

    for (Entity *e : entities)
    {
        if (e->getFaction() == Faction::Enemy && e->is_alive())
        {
            if (e->get_hp() < e->get_max_hp() * 0.7)
            {
                allyLowHp = true;
                break;
            }
        }
    }

    ai.update(get_hp(), isHealer && hasItems(), has_focus(), allyLowHp);

    return ai.decideAction();
}

void Enemy::info() const
{
    std::cout << get_name()
              << " HP: " << get_hp();

    if (get_focus() > 0)
        std::cout << " [Focus: " << get_focus() << "]";

    std::cout << std::endl;
}

void Enemy::set_isHealer(bool healer)
{
    isHealer = healer;
};

int Entity::get_mana() const
{
    return resources.mana;
}

void Entity::add_mana(int amount)
{
    resources.mana += amount;

    if (resources.mana > 10) // кап можно потом вынести
        resources.mana = 10;
}

bool Entity::spend_mana(int amount)
{
    if (resources.mana < amount)
        return false;

    resources.mana -= amount;
    return true;
}
