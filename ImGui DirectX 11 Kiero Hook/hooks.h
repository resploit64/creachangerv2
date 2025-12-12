#pragma once
#include "hook_manager.h"

// ============================================
// EXAMPLE FUNCTION SIGNATURES
// ============================================

// Example function types
typedef void(__fastcall* DrawObject_t)(void* thisptr, int a1, int a2);
typedef void(__fastcall* RenderBatchList_t)(void* thisptr);
typedef float(__fastcall* GetFOV_t)(void* pCameraServices);
typedef void*(__fastcall* OnAddEntity_t)(void* pThis, void* pInstance, void* hHandle);
typedef float(__fastcall* GetAsspectRatio)(void* thisptr, int width, int height);
typedef void(__fastcall* FrameStage)(void* _this, int curStage);
typedef void* (*EnableCursor)(void* rcx, bool active);
typedef bool(__thiscall* EquipItemInLoadout_t)(void* thisptr, int iTeam, int iSlot, uint64_t iItemID);
typedef char(__fastcall* tSub_1802BD580)(long long a1, long long a2);

//48 89 54 24 ? 55 41 55
// ============================================
// DECLARE HOOK OBJECTS
// ============================================�

namespace H
{
     inline CBaseHookObject<EnableCursor> hkEnableCursor = {};
	 inline CBaseHookObject<EquipItemInLoadout_t> hkEquipItemInLoadout = {};

	 inline CBaseHookObject<FrameStage> hkFrameStage = {};
	 inline CBaseHookObject<DrawObject_t> hkDrawObject = {};
	 inline CBaseHookObject<RenderBatchList_t> hkRenderBatchList = {};
	 inline CBaseHookObject<GetFOV_t> hkGetFOV = {};
	 inline CBaseHookObject<OnAddEntity_t> hkOnAddEntity = {};
	 inline CBaseHookObject<GetAsspectRatio> hkGetAsspectRatio = {};

	 inline CBaseHookObject<tSub_1802BD580> hkDispatchEvent = {};
}

// ============================================
// DETOUR FUNCTIONS
// ============================================

namespace D
{
	void hkEnableCursor(void* rcx, bool active);
	void __fastcall GetScreenAspectRatio(void* thisptr, int width, int height);
	void __fastcall DrawObject(void* thisptr, int a1, int a2);
	void __fastcall hkFrameStage(void* _this, int curStage);
	bool __fastcall hkEquipItemInLoadout(void* thisptr, int iTeam, int iSlot, uint64_t iItemID);
	char __fastcall hkSub_1802BD580(long long a1, long long a2);
}
