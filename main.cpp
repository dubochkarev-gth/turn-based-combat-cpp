#include <iostream>
#include <string>
#include <random>
#include <algorithm>
#include <vector>

using namespace std;

//Forward Declaration;

class Entity;

//CONSTANTS

constexpr float FOCUS_BONUS_MULTIPLIER = 1.5f;
constexpr int CRIT_CHANCE_PERCENT = 20;
constexpr float CRIT_MULTIPLIER = 2.0f;
constexpr float BLOCK_BONUS_MULTIPLIER = 0.5f;
constexpr int HEAL_AMOUNT = 7;

// --------------------
// Random helper
// --------------------
int randomInt(int min, int max) {
    static mt19937 gen(random_device{}());
    uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

// clear screen helper
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
};

//ENUMS

enum class ActionType{
    Attack,
    Heal,
    Block
};

enum class AIState {
    Aggressive,
    Defensive,
    Desperate
};

//STRUCTS

struct ActionResult {
    ActionType type;
    string actor;
    string target;

    int damage = 0;
    int healed = 0;
    bool isCritical = false;
    bool targetDied = false;
    bool usedFocus = false;
};

struct PlannedAction {
    Entity* actor = nullptr;
    ActionType type;
    Entity* target = nullptr;
};

struct BattleLog {
    vector<ActionResult> actions;

    void clear() {
        actions.clear();
    }

    void add(const ActionResult& result) {
        actions.push_back(result);
    }
};

struct Stats {
    int baseInitiative = 0;
    // future:
    // int speed;
    // int agility;
};

// --------------------
// Base Entity class
// --------------------
class Entity {
protected:
    int hp;
    int max_hp;
    string name;
    bool isBlocking = false;
    int focus = 0;
    Stats stats;

public:
    Entity(string n, int h, int baseInitiative)
        : hp(h), max_hp(h), name(n) {
            stats.baseInitiative = baseInitiative;
        }

    int get_hp() const {
        return hp;
    }

    string get_name() const {
        return name;
    }

    bool has_focus() const{
        return focus > 0;
        }

    void add_focus(int amount){
        focus += amount;
    }

    void consume_focus(){
        focus = 0;
    }

    int take_damage(int dmg) {
        int finalDamage = 0;
        
        if(isBlocking){
            finalDamage = dmg*BLOCK_BONUS_MULTIPLIER;
            isBlocking = false;
        }
        else{
            finalDamage = dmg;
        }
        hp -= finalDamage;
        if (hp < 0) {
            hp = 0;
        }

        return finalDamage;
    }
    
    bool is_alive() const {
        return hp > 0;
    }

    virtual int get_attack_power() const {
        return 10;
    }

    virtual void info() const = 0;

    ActionResult attack(Entity& target){
        ActionResult result;

        result.type = ActionType::Attack;
        result.actor = name;
        result.target = target.get_name();

        int dmg = randomInt(1, get_attack_power());

    // крит — пока простой
        bool crit = randomInt(1, 100) <= CRIT_CHANCE_PERCENT;
    if (crit) {
        dmg *= CRIT_MULTIPLIER;
        result.isCritical = true;
    }
    if (has_focus()) {
        dmg *= FOCUS_BONUS_MULTIPLIER;
        consume_focus();
        result.usedFocus = true;
    }

    result.damage = target.take_damage(dmg);
    result.targetDied = !target.is_alive();

    return result;
    }

    ActionResult block(){
        ActionResult result;
        result.type = ActionType::Block;
        result.actor = name;

        isBlocking = true;
        add_focus(1);

        return result;
    }

    int calculateInitiative() const {
        return stats.baseInitiative;
    }

    int getInitiative() const {
        return calculateInitiative();
    }

    virtual ActionType decideAction(Entity& target) {
        return ActionType::Block;
    }

    int take_heal(int amount) {
        if (!is_alive())
            return 0;

        int before = hp;
        hp = min(hp + amount, max_hp);
        return hp - before;
    }

    ActionResult heal() {
        ActionResult result;
        result.type = ActionType::Heal;
        result.actor = name;

        result.healed = take_heal(HEAL_AMOUNT);

        return result;
    }

    int get_focus() const {
        return focus;
    }

};

// --------------------
// Player
// --------------------
class Player : public Entity {
protected:
    int weapon_bonus;

public:
    Player(string name, int hp, int baseInitiative, int weapon)
        : Entity(name, hp, baseInitiative), weapon_bonus(weapon) {}

    int get_attack_power() const override {
        return 10 + weapon_bonus;
    }

    void info() const override {
        cout << get_name() << " HP: " << get_hp();
        
        if (get_focus() > 0)
        cout << " [Focus: " << get_focus() << "]";

        cout<< endl;
    }

    ActionType decideAction(Entity& target) override {
        int playerChoice = 0;

        while (playerChoice < 1 || playerChoice > 3) {
            cout << "Player make a choice:\n";
            cout << "1 - attack\n2 - block\n3 - heal\n";
            cin >> playerChoice;
        }

        if (playerChoice == 1) return ActionType::Attack;
        if (playerChoice == 2) return ActionType::Block;
        return ActionType::Heal;
    }
};

// Decision making for Enemy

class EnemyAI{
    private:
    AIState state = AIState::Aggressive;

    public:
    void update(const Entity& self)
    {
        int hp = self.get_hp();

        if (hp < 20)
            state = AIState::Desperate;
        else if (hp < 40)
            state = AIState::Defensive;
        else
            state = AIState::Aggressive;
    }

    ActionType decideAction(const Entity& self) const 
    {
        switch (state) {
            case AIState::Aggressive:
                return ActionType::Attack;

            case AIState::Defensive:{
                int roll = randomInt(0, 2);
                if (roll == 0)
                    return ActionType::Attack;
                if (roll == 1)
                    return ActionType::Heal;
                else 
                    return ActionType::Block;
            }

            case AIState::Desperate:
                return self.has_focus()
                    ? ActionType::Attack
                    : ActionType::Block;
    }
    return ActionType::Attack;

    }
}; 

// --------------------
// Enemy
// --------------------
class Enemy : public Entity {
protected:
    int base_attack;
    int strength;
    EnemyAI ai;

public:
    Enemy(string name, int hp,  int baseInitiative, int baseAtk, int str)
        : Entity(name, hp, baseInitiative),
          base_attack(baseAtk),
          strength(str) {}

    int get_attack_power() const override {
        return base_attack + strength;
    }

    void info() const override {
        cout << get_name() << " HP: " << get_hp();
        
        if (get_focus() > 0)
            cout << " [Focus: " << get_focus() << "]";
        
        cout << endl;
    }

    ActionType decideAction(Entity& target) override {
    ai.update(*this);
    return ai.decideAction(*this);
    }
};

//SCREEN DRAW AFTER CALCULATION

void renderBattleScreen(
    const Player& p,
    const Enemy& e,
    const BattleLog& log,
    const vector<Entity*>& turnOrder
) {
    clearScreen();

    cout << "====== BATTLE ======\n\n";

    p.info();
    e.info();

    cout << "\n--------------------\n";

    cout << "\n--- Initiative order ---\n\n";

    for (const Entity* ent : turnOrder){
        
        if (!ent->is_alive())
        continue;

        cout<< ent->get_name() << " (" << ent->getInitiative() << ")"
        << "-----";
        }
    
    cout << "\n";

    cout << "\n--- Last turn ---\n";

for (const ActionResult& r : log.actions) {

    if (r.type == ActionType::Attack) {
        cout << r.actor << " hits " << r.target
             << " for " << r.damage;

        if (r.isCritical)
            cout << " (CRITICAL)";

        if (r.usedFocus)
            cout << " (FOCUSED)";
    }

    if (r.type == ActionType::Block) {
        cout << r.actor << " blocks part of incoming damage";
    }

    if (r.type == ActionType::Heal) {
        cout << r.actor << " heals for "
             << r.healed << " HP";
    }

    cout << endl;

    if (r.targetDied) {
        cout << r.target << " is defeated!" << endl;
    }
}

    cout << "\n--------------------\n";
}

//Initiative calculator

void buildTurnOrder(const vector<Entity*>& entities,
                    vector<Entity*>& turnOrder) {
    turnOrder = entities;

    sort(turnOrder.begin(), turnOrder.end(),
        [](Entity* a, Entity* b) {
            return a->getInitiative() > b->getInitiative();
        });
}

//Executor for battle

void executeAction(const PlannedAction& action,
                   BattleLog& log)
{
    if (!action.actor || !action.actor->is_alive())
        return;

    ActionResult result;

    switch (action.type) {
        case ActionType::Attack:
            result = action.actor->attack(*action.target);
            break;

        case ActionType::Block:
            result = action.actor->block();
            break;

        case ActionType::Heal:
            result = action.actor->heal();
            break;
    }

    log.add(result);
};

Entity* findFirstAliveEnemy(
    Entity* actor,
    const vector<Entity*>& entities
) {
  for (Entity* e : entities) {
        if (e != actor && e->is_alive())
            return e;
    }
    return nullptr;
}

// Decision function

vector<PlannedAction> planTurn(
    const vector<Entity*>& turnOrder,
    const vector<Entity*>& entities
){
    vector<PlannedAction> plannedActions;

    for (Entity* actor : turnOrder) {

        if(!actor->is_alive())
        continue;
        

        Entity* target = findFirstAliveEnemy(actor, entities);

        if(!target)
        continue;

        PlannedAction action;
        action.actor = actor;
        action.type = actor->decideAction(*target);
        action.target = target;

        plannedActions.push_back(action);
        }
        
    return plannedActions;
};

// --------------------
// Battle
// --------------------
void runBattle(Player& p, Enemy& e) {

    BattleLog log;

    vector<Entity*> entities = { &p, &e };
    vector<Entity*> turnOrder;

    while (true) {
    
    buildTurnOrder(entities, turnOrder);
    log.clear();
    vector<PlannedAction> plannedActions=
    planTurn(turnOrder, entities);

    // -------- Execution phase --------
    for (const PlannedAction& action : plannedActions) {

        executeAction(action, log);

    }

    renderBattleScreen(p, e, log, turnOrder);

    if (!p.is_alive()) {
        cout << "\n=== Battle Finished ===\n";
        cout << "Winner: " << e.get_name() << endl;
        break;
    }

    if (!e.is_alive()) {
        cout << "\n=== Battle Finished ===\n";
        cout << "Winner: " << p.get_name() << endl;
        break;
    }

    cout << "\nPress Enter to continue...";
    cin.ignore();
    cin.get();
    }
}

// --------------------
// Main
// --------------------
int main() {
    Player hero("Dark_Avanger", 100, 10, 5);

    Enemy kobold("Sneaky_Kody", 50, 15, 5, 3);
    Enemy orc("Gazkul_Trakka", 90, 9, 7, 4);

    runBattle(hero, orc);

    return 0;
}