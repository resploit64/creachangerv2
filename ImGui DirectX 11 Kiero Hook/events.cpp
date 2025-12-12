#include "CCPlayerInventory.h"
#include "entity.h"


CCSPlayerController* CGameEventHelper::GetPlayerController()
{
        if (!Event)
                return nullptr;

        int controller_id{};

        CBuffer buffer;
        buffer.name = "userid";

        Event->GetControllerId(controller_id, &buffer);

        if (controller_id == -1)
                return nullptr;

        return (CCSPlayerController*)I::gameresource->pGameEntitySystem->Get(controller_id + 1);
}


CCSPlayerController* CGameEventHelper::GetAttackerController()
{
        if (!Event)
                return nullptr;

        int controller_id{};

        CBuffer buffer;
        buffer.name = "attacker";

        Event->GetControllerId(controller_id, &buffer);

        if (controller_id == -1)
                return nullptr;

        return (CCSPlayerController*)I::gameresource->pGameEntitySystem->Get(controller_id + 1);
}



void CEvents::PlayerDeath(CGameEvent* event)
{
        auto attacker = event->GetEventHelper().GetAttackerController();
        auto local = CCSPlayerController::GetLocalPlayerController();
        if (!attacker || !local)
                return;

        if (attacker == local) {
                const char* weapon_name = event->GetString(cUltStringToken("weapon"));
                printf("debug %s\n", weapon_name);
                if (weapon_name &&
                        (strcmp(weapon_name, "knife") == 0 || strcmp(weapon_name, "knife_t") == 0)) {


                        auto pawn = C_CSPlayerPawn::GetLocalPawn();

                        auto local_inventory = CCSPlayerInventory::GetInstance();
                        const auto econ_item_view = local_inventory->GetItemInLoadout(static_cast<int32_t>(pawn->team()), 0);
                        if (!econ_item_view)
                                return;
                        auto econ_item_definition = econ_item_view->get_static_data();
                        if (!econ_item_definition)
                                return;

                        const char* simple = econ_item_definition->get_simple();
                        std::string weapon_name = simple;

                        if (weapon_name.rfind("weapon_", 0) == 0)
                                weapon_name.erase(0, 7);
                        event->SetString(cUltStringToken("weapon"), weapon_name.c_str());

                }
        }
}

void CEvents::LocalPlayerDeath(CGameEvent* event) {

}
// main 
void CEvents::FireGameEvent(CGameEvent* event) {
        if (!I::engine->is_connected() || !I::engine->is_in_game())
                return;
        std::string name = event->GetName();

        if (name.find("player_death") != std::string::npos)
                PlayerDeath(event);
}


bool CEvents::Intilization()
{


        I::eventmanager->AddListener(this, "player_death", false);
        if (!I::eventmanager->FindListener(this, "player_death"))
                return false;






        Events->inited = true;

        return true;
}