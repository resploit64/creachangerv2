#include "hook_manager.h"
#include "logger.h"
#include "hooks.h"
#include "memory.h"
#include "sdk.h"
#include "interfaces.h"
#include "entity.h"
#include "skins.h"
#include "CCPlayerInventory.h"
namespace H
{
	bool Setup()
	{
		// Initialize MinHook
		if (const MH_STATUS status = MH_Initialize(); status != MH_OK)
		{
			printf("[H] Failed to initialize MinHook: %s\n", MH_StatusToString(status));
			return false;
		}

		if (!hkGetAsspectRatio.Create(reinterpret_cast<void*>(M::FindPattern(L"engine2.dll", "48 89 5C 24 08 57 48 83 EC 20 8B FA 48 8D 0D")), (void*)&D::GetScreenAspectRatio))
		{
			printf("[---] Failed to create GetScreenAspectRatio hook\n");
			return false;
		}
		if (!hkFrameStage.Create(M::get_v_method(I::iclient, 36), (void*)&D::hkFrameStage)) {
			printf("[---] Failed to create hkFrameStage hook\n");
			return false;
		}
		if (!hkEnableCursor.Create(M::get_v_method(I::inputsystem, 76), (void*)&D::hkEnableCursor))
		{
			printf("[---] Failed to create EnableCursor hook\n");
			return false;
		}

		if (!hkEquipItemInLoadout.Create(reinterpret_cast<void*>(M::FindPattern(L"client.dll","48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 89 54 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 0F B7 F2" )), (void*)&D::hkEquipItemInLoadout)) 
		{
			printf("[---] Failed to create hkEquipItemInLoadout hook!\n");
		}


		return true;
	}

	void Destroy()
	{
		printf("[H] Destroying hooks...\n");

		// TODO: Remove your hooks here
		// Example:
		// if (hkYourFunction.IsHooked())
		// {
		//     hkYourFunction.Remove();
		//     printf("[+] YourFunction hook removed\n");
		// }
		hkGetAsspectRatio.Remove();
		hkEquipItemInLoadout.Remove();
		hkEnableCursor.Remove();
		hkFrameStage.Remove();
		if (const MH_STATUS status = MH_Uninitialize(); status != MH_OK)
		{
			printf("[---] Failed to uninitialize MinHook: %s\n", MH_StatusToString(status));
			return;
		}

		printf("[H] MinHook uninitialized successfully\n");
	}
}


namespace D
{
	void __fastcall DrawObject(void* thisptr, int a1, int a2)
	{
		// Example detour function
		printf("[H] DrawObject called with a1=%d, a2=%d\n", a1, a2);

		H::hkDrawObject.GetOriginal()(thisptr, a1, a2);
	}
	void __fastcall GetScreenAspectRatio(void* thisptr, int width, int height) {

		if (SDK::g_ScreenHeight != height || SDK::g_ScreenWidth != width) {
			SDK::g_ScreenHeight = height;
			SDK::g_ScreenWidth = width;
			printf("[D] Screen resized: width=%d, height=%d\n", width, height);
		}
		H::hkGetAsspectRatio.GetOriginal()(thisptr, width, height);

	}
	void __fastcall hkFrameStage(void* _this, int curStage)
	{
		//  printf("hk %d\n", curStage);
		H::hkFrameStage.GetOriginal()(_this, curStage);
		if (!I::engine && !I::engine->is_in_game()) return;
		if (curStage == 6) {



			C_CSPlayerPawn* local_pawn = C_CSPlayerPawn::GetLocalPawn();
			if (!local_pawn) return;
			CCSPlayerInventory* inventory = CCSInventoryManager::GetInstance()->GetLocalInventory();

			C_CS2HudModelWeapon* viewModel = local_pawn->GetViewModel();
			S::set_agent(curStage);
			S::ApplyKnifeSkins(local_pawn, inventory, viewModel);
			S::ApplyWeaponSkins(local_pawn, inventory, viewModel);
			S::ApplyGloves();
			//skins::set_knife(curStage);
		}
	}

	void hkEnableCursor(void* thisptr, bool bEnable)
	{

		H::hkEnableCursor.GetOriginal()(thisptr, bEnable);
	}

	bool __fastcall hkEquipItemInLoadout(void* thisptr, int iTeam, int iSlot, uint64_t iItemID) {

		// Вызываем оригинальную функцию
		bool result = H::hkEquipItemInLoadout.GetOriginal()(thisptr, iTeam, iSlot, iItemID);
		S::onEquipItemInLoadout(thisptr, iTeam, iSlot, iItemID);

		return result;
	}

	char __fastcall hkSub_1802BD580(long long a1, long long a2)
	{
		// Логируем вызовы
		printf("Hooked! a1 = %lld, a2 = %lld\n", a1, a2);

		// Можно менять аргументы перед вызовом оригинала
		// a1 = 123;

		// вызываем оригинальную
		char result = H::hkDispatchEvent.GetOriginal()(a1, a2);

		// можно изменить результат
		// result = 1;

		return result;
	}

}