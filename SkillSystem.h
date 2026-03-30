// ---SkillSystem.h ---

#pragma once
#include "Skill.h"
#include "Entity.h"

class SkillSystem
{
public:
    static ActionResult execute(const Skill& skill, Entity& actor, Entity& target);
};