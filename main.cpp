#include <iostream>
#include <string>
#include <random>

using namespace std;

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

enum class ActionType{
    Attack,
    Heal,
    Block,
    Skip
};

struct ActionResult {
    ActionType type;
    string actor;
    string target;

    int damage = 0;
    bool isCritical = false;
    bool targetDied = false;
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

public:
    Entity(string n, int h)
        : hp(h), name(n) {}

    int get_hp() const {
        return hp;
    }

    string get_name() const {
        return name;
    }

    int take_damage(int dmg) {
        int finalDamage = 0;
        const float BLOCK_BONUS_MULTIPLIER = 0.5f;

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
        bool crit = randomInt(1, 100) <= 20;
    if (crit) {
        dmg *= 2;
        result.isCritical = true;
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
};

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

    bool attackBonusReady = false;
        
    int playerChoice = 0;
  
    BattleLog log;
    
    while (true) {
        renderBattleScreen(p, e, log);

        cout << "Player make a choice: 1 - attack, 2 - block (reduce incoming damag)." << endl;
        cout << "Your choice?" << endl;
        cin >> playerChoice;
        
        log.clear();
              
        while (playerChoice != 1 && playerChoice != 2){
            cout << "Wrong Input!!!!!!" << endl;
            cout << "Player make a choice: 1 - attack, 2 - block (reduce incoming damag)." << endl;
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
        
        log.enemyAction = e.attack(p);
        log.hasEnemyAction = true;

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
