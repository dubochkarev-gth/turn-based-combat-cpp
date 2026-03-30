//--- ItemSystem.cpp

#include "ItemSystem.h"
#include "Inventory.h"

ActionResult ItemSystem::useItem(Entity& actor)
{
    ActionResult result;
    result.type = ActionType::UseItem;
    result.actor = actor.get_name();
    result.target = "";

    Inventory* inv = actor.getInventory();

    if (!inv)
        return result;

    auto itemOpt = inv->takeFirst();
    if (!itemOpt)
        return result;

    Item item = *itemOpt;
    result.itemName = item.name;

    if (item.type == ItemType::Heal)
    {
        result.healedPlanned = static_cast<int>(item.power * 0.7f);
    }

    return result;
}