#include <iostream>
#include <string>
#include <random>

using namespace std;

//CONSTANTS

constexpr float FOCUS_BONUS_MULTIPLIER = 1.5f;
constexpr int CRIT_CHANCE_PERCENT = 20;
constexpr float CRIT_MULTIPLIER = 2.0f;
constexpr float BLOCK_BONUS_MULTIPLIER = 0.5f;

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
    Block,
    Skip
};

//STRUCTS

struct ActionResult {
    ActionType type;
    string actor;
    string target;

    int damage = 0;
    bool isCritical = false;
    bool targetDied = false;
    bool usedFocus = false;
};

struct BattleLog {
    ActionResult playerAction;
    ActionResult enemyAction;
    bool hasPlayerAction = false;
    bool hasEnemyAction = false;

    void clear() {
        hasPlayerAction = false;
        hasEnemyAction = false;
    }
};

// --------------------
// Base Entity class
// --------------------
class Entity {
protected:
    int hp;
    string name;
    bool isBlocking = false;
    int focus = 0;

public:
    Entity(string n, int h)
        : hp(h), name(n) {}

    int get_hp() const {
        return hp;
    }

    string get_name() const {
        return name;
    }

    bool has_focus() const{
        if (focus > 0) return true;
        else return false;
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
};

// --------------------
// Player
// --------------------
class Player : public Entity {
protected:
    int weapon_bonus;

public:
    Player(string name, int hp, int weapon)
        : Entity(name, hp), weapon_bonus(weapon) {}

    int get_attack_power() const override {
        return 10 + weapon_bonus;
    }

    void info() const override {
        cout << get_name() << " HP: " << get_hp() << endl;
    }
};

// --------------------
// Enemy
// --------------------
class Enemy : public Entity {
protected:
    int base_attack;
    int strength;

public:
    Enemy(string name, int hp, int baseAtk, int str)
        : Entity(name, hp),
          base_attack(baseAtk),
          strength(str) {}

    int get_attack_power() const override {
        return base_attack + strength;
    }

    void info() const override {
        cout << get_name() << " HP: " << get_hp() << endl;
    }

    //Here will be extencion for enemy behavior.
};

//SCREEN DRAW AFTER CALCULATION

void renderBattleScreen(
    const Player& p,
    const Enemy& e,
    const BattleLog& log
) {
    clearScreen();

    cout << "====== BATTLE ======\n\n";

    p.info();
    e.info();

    cout << "\n--------------------\n";

    cout << "\n--- Last turn ---\n";

    if (log.hasPlayerAction) {
        const ActionResult& r = log.playerAction;
        if(r.type == ActionType::Attack){
            cout << r.actor << " hits " << r.target
            << " for " << r.damage;

        if (r.isCritical)
            cout << " (CRITICAL)";

        if (r.usedFocus)
            cout << " (FOCUSED)";

        }
      

        if(r.type == ActionType::Block){
            cout << r.actor << " blocks "
         << "part of incoming damage ";
        }

        cout << endl;

    if (r.targetDied)
        cout << r.target << " is defeated!" << endl;
    }

    if (log.hasEnemyAction) {
    const ActionResult& v = log.enemyAction;
    cout << v.actor << " hits " << v.target
         << " for " << v.damage;

    if (v.isCritical)
        cout << " (CRITICAL)";

    cout << endl;

    if (v.targetDied)
        cout << v.target << " is defeated!" << endl;
    }

    cout << "\n--------------------\n";
}

// --------------------
// Battle
// --------------------
void Battle(Player& p, Enemy& e) {
          
    int playerChoice = 0;
  
    BattleLog log;
    
    while (true) {
        renderBattleScreen(p, e, log);

        //Couts should be in function not just text in battle.

        cout << "Player make a choice: 1 - attack, 2 - block (reduce incoming damage and add focus)." << endl;
        cout << "Your choice?" << endl;
        cin >> playerChoice;
        
        log.clear();
              
        while (playerChoice != 1 && playerChoice != 2){
            cout << "Wrong Input!!!!!!" << endl;
            cout << "Player make a choice: 1 - attack, 2 - block (reduce incoming damage and add focus)." << endl;
            cout << "Your choice?" << endl;
            cin >> playerChoice;
        } 
              
        if (playerChoice == 1){
            log.playerAction = p.attack(e);
            log.hasPlayerAction = true;
        }
        
        if (playerChoice == 2){
           log.playerAction = p.block();
           log.hasPlayerAction = true; 
        }
        
        // Here should be AI swithc for enemy behavior.
        log.enemyAction = e.attack(p);
        log.hasEnemyAction = true;
        
        // Break should be replaced, because of it last draw of screen not work.
        if (!p.is_alive()) {
            cout << endl;
            cout << "=== Battle Finished ===" << endl;
            cout << "Winner: " << e.get_name() << endl;
            break;
        }

        if (!e.is_alive()) {
            cout << endl;
            cout << "=== Battle Finished ===" << endl;
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
    Player hero("Dark_Avanger", 100, 5);

    Enemy kobold("Sneaky_Kody", 50, 5, 3);
    Enemy orc("Gazkul_Trakka", 80, 7, 4);

    Battle(hero, orc);

    return 0;
}
