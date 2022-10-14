/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
 
#include "CDetour/detours.h"
#include <iclient.h>
#include <iserver.h>
#include <icvar.h>
#include "netmessages.h"
#include <tier1/interface.h>

#define GAMECONFIG_FILE "voicehook"

VoiceHook g_VoiceHook;

SMEXT_LINK(&g_VoiceHook);

IGameConfig *g_pGameConf = nullptr;
CDetour *g_pSV_BroadcastVoiceDataDetor = nullptr;
IForward *g_pVoiceToClientForward = nullptr;
IServer *g_pIServer = nullptr;
ConVar *g_pVoiceEnable = nullptr;

DETOUR_DECL_STATIC3(SV_BroadcastVoiceData, void, IClient *, cl, int, nBytes, char *, data) {
	if (!g_pVoiceEnable->GetInt()) return;
	
	SVC_VoiceData voiceData;
	
	int iClient = cl->GetPlayerSlot();
	
	int iBytes = nBytes * 8;
	
	voiceData.m_nFromClient = iClient;
	voiceData.m_DataOut = data;
	
	for (int i = 0; i < g_pIServer->GetClientCount(); i++) {
		IClient *pDestClient = g_pIServer->GetClient(i);
		
		if (pDestClient == cl || !pDestClient->IsActive() || !pDestClient->IsHearingClient(iClient)) continue;
		
		//FORWARD
		g_pVoiceToClientForward->PushCell(iClient+1);
		g_pVoiceToClientForward->PushCell(pDestClient->GetPlayerSlot()+1);
		
		int bResult = 1;
		g_pVoiceToClientForward->PushCellByRef(&bResult);
	
		cell_t result = Pl_Continue;
		g_pVoiceToClientForward->Execute(&result);

		if (result >= Pl_Handled) return;
		if (!bResult && result == Pl_Changed) continue;
		
		voiceData.m_nLength = iBytes;
		
		pDestClient->SendNetMsg(voiceData);
	}
}

bool VoiceHook::SDK_OnLoad(char *error, size_t maxlength, bool late) {
	if (!gameconfs->LoadGameConfigFile(GAMECONFIG_FILE, &g_pGameConf, error, maxlength)) {
		g_pSM->Format(error, maxlength, "Failed to load gameconfig: %s", GAMECONFIG_FILE);
		
		return false;
	}

	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
	
	void *pAddr = nullptr;
	if (!g_pGameConf->GetMemSig("sv", &pAddr) || !pAddr) {
		g_pSM->Format(error, maxlength, "Failed get offset of sv variable");

		return false;
	}
	
	g_pIServer = reinterpret_cast<IServer *>(pAddr);
	
	ICvar *cvar = nullptr;
	Sys_LoadInterface("engine", VENGINE_CVAR_INTERFACE_VERSION, nullptr, (void**)&cvar);
	
	if (!cvar) {
		g_pSM->Format(error, maxlength, "ICVAR not found (%s)", VENGINE_CVAR_INTERFACE_VERSION);
		
		return false;
	}
	
	g_pVoiceEnable = cvar->FindVar("sv_voiceenable");
	if (!g_pVoiceEnable) {
		g_pSM->Format(error, maxlength, "Failed to find cvar: sv_voiceenable");
		
		return false;
	}
	
	return true;
}

void VoiceHook::SDK_OnAllLoaded() {
	if (g_pSV_BroadcastVoiceDataDetor == nullptr) {		
		g_pSV_BroadcastVoiceDataDetor = DETOUR_CREATE_STATIC(SV_BroadcastVoiceData, "SV_BroadcastVoiceData");
		if (g_pSV_BroadcastVoiceDataDetor) g_pSV_BroadcastVoiceDataDetor->EnableDetour();
		else g_pSM->LogError(myself, "Failed to setup ProcessVoiceData detour");
		g_pVoiceToClientForward = forwards->CreateForward("VoiceHook_OnClientVoiceTo", ET_Event, 3, nullptr, Param_Cell, Param_Cell, Param_CellByRef);
	}
}

void VoiceHook::SDK_OnUnload() {
	forwards->ReleaseForward(g_pVoiceToClientForward);

	if (g_pSV_BroadcastVoiceDataDetor) {
		g_pSV_BroadcastVoiceDataDetor->Destroy();
		g_pSV_BroadcastVoiceDataDetor = nullptr;
	}

	gameconfs->CloseGameConfigFile(g_pGameConf);
}
