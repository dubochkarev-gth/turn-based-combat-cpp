// --- Entity.h ---
#pragma once
#include <string>
#include <memory>
#include "CombatTypes.h"
#include "Item.h"
#include "Resources.h"
#include "Inventory.h"

enum class Faction
{
    Player,
    Enemy
};

struct Stats
{
    int baseInitiative = 0;
};

class Entity
{
protected:
    int hp;
    int max_hp;
    std::string name;
    bool isBlocking = false;
    int focus = 0;
    Stats stats;
    Faction faction;
    std::unique_ptr<Inventory> inventory;
    float threat = 0.0f;
    float damageMultiplier = 1.0f;
    float threatMultiplier = 1.0f;
    float blockMultiplierFromEquip = 1.0f;
    Resources resources;
    bool hasTauntSkill = false;

public:
    Entity(std::string n, int h, int baseInitiative, Faction f);

    int get_hp() const;
    int get_max_hp () const;
    int get_focus() const;
    std::string get_name() const;
    Faction getFaction() const;

    bool is_alive() const;

    void heal(int amount);
    int receive_damage(int amount);

    void set_blocking(bool);
    void add_focus(int);
    void consume_focus();

    void add_threat(float amount);
    float get_threat() const;
    void decay_threat(float factor);
    void reset_threat();

    virtual int get_attack_power() const;
    int getInitiative() const;

    bool has_focus() const;

    virtual ActionType decideAction(const std::vector<Entity*>& entities);
    virtual ActionResult attack(Entity& target);
    virtual ActionResult block();
    virtual void info() const;
    virtual ActionResult taunt();

    void attachInventory(std::unique_ptr<Inventory> inv){
        inventory = std::move(inv);
    }

    Inventory* getInventory(){
        return inventory.get();
    }

    bool hasItems() const;
    void equip(const Item& item);

    int get_guard() const;
    void add_guard(int amount);
    bool spend_guard(int amount);
    bool has_taunt() const;

    int get_momentum() const;
    void add_momentum(int amount);
    bool spend_momentum(int amount);
    void reset_momentum();
};

class Player : public Entity
{
protected:
    int weapon_bonus;
    bool autoMode = false;

public:
    Player(std::string name, int hp, int baseInitiative, int weapon);

    int get_attack_power() const override;
    void setAutoMode(bool value);
    void info() const override;
    ActionType decideAction(const std::vector<Entity*>& entities) override;
};

enum class AIState
{
    Aggressive,
    Defensive,
    Desperate
};

class EnemyAI
{
private:
    AIState state = AIState::Aggressive;
    bool canHeal = false;
    bool hasFocus = false;
    bool allyLowHp = false;

public:
    void update(int hp, bool canHealNow, bool hasFocusNow, bool curAllyLowHp);
    ActionType decideAction() const;
};

class Enemy : public Entity
{
protected:
    int base_attack;
    int strength;
    bool isHealer = false;

public:
    Enemy(std::string name, int hp, int baseInitiative, int baseAtk, int str);

    EnemyAI ai;

    int get_attack_power() const override;

    void set_isHealer(bool healer);

    ActionType decideAction(const std::vector<Entity*>& entities) override;
    void info() const override;
    
};
