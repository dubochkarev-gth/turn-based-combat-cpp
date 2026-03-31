// ---SkillSystem.cpp ---

#include "SkillSystem.h"

extern int randomInt(int min, int max);

ActionResult SkillSystem::execute(const Skill &skill, Entity &actor, Entity &target)
{
    ActionResult result;

    result.type = skill.type;
    result.actor = actor.get_name();
    result.target = target.get_name();

    // ======================
    // RESOURCE CHECK
    // ======================
    bool ok = false;

    switch (skill.resource)
    {
    case ResourceType::Mana:
        ok = actor.spend_mana(skill.cost);
        break;

    case ResourceType::Momentum:
        ok = actor.spend_momentum(skill.cost);
        break;

    case ResourceType::Guard:
        ok = actor.spend_guard(skill.cost);
        break;

    default:
        ok = true;
    }

    if (!ok)
    {
        result.cancelled = true;
        return result;
    }

    // ======================
    // EFFECT SYSTEM
    // ======================
    switch (skill.type)
    {
    case ActionType::Attack:
    case ActionType::Burst:
    {
        int dmg = randomInt(1, actor.get_attack_power());

        bool usedFocus = false;

        if (actor.has_focus())
        {
            dmg = static_cast<int>(dmg * FOCUS_BONUS_MULTIPLIER);
            actor.consume_focus();
            usedFocus = true;
        }

        dmg = static_cast<int>(dmg * skill.powerMultiplier);

        result.damagePlanned = dmg;
        result.usedFocus = usedFocus;

        break;
    }

    case ActionType::Heal:
    {
        int heal = randomInt(6, 12);
        heal = static_cast<int>(heal * skill.powerMultiplier * 0.8f);
        result.healedPlanned = heal;
        break;
    }

    case ActionType::Taunt:
    {
        const float flatThreat = 0.3f;
        actor.add_threat(flatThreat);
        break;
    }

    case ActionType::Block:
    {

        break;
    }

    default:
        break;
    }

    return result;
}