/*
    gm_pvs
    By Spacetech
	Original by Darkspider / Train
*/

#include <iostream>
#include <map>

#include <windows.h>
#include <gmodinterface/gmluamodule.h>

// Probably don't even need half of these for this...
#include <interface.h>
#include "eiface.h"
#include "ehandle.h"
#include "isaverestore.h"
#include "touchlink.h"
#include "groundlink.h"
#include "variant_t.h"
#include "predictableid.h"
#include "shareddefs.h"
#include "util.h"
#include "predictable_entity.h"
#include "takedamageinfo.h"
#include "baseentity_shared.h"

#include "engine/IEngineSound.h"

#include "vfnhook.h"

#include "mrecpfilter.h"

#include "steam/steamclientpublic.h"

#include "tier0/memdbgon.h"

GMOD_MODULE(Init, Shutdown);

float UpdateTime = 1.0;
float PVS_CONNECTED = 26.0;

edict_t *pBaseEdict = NULL;
int EntityRef, HookCallRef, EntIndexRef;

ILuaInterface *gLua = NULL;
IVEngineServer *engine = NULL;
IEngineSound *sound = NULL;
IServerGameEnts *gameents = NULL;

struct TransmitInfo_t
{
	float NextUpdate;
	int nNewEdicts;
	unsigned short *pNewEdictIndices;
};

static std::map<int, TransmitInfo_t> ClientTransmitInfo;

static const char *sAlwaysInstance[] =
{
	"bodyque",

	"team_manager",
	"scene_manager",
	"player_manager",

	"viewmodel",
	"predicted_viewmodel",

	"prop_door_rotating",

	"soundent",
	"water_lod_control",

	"", // END Marker

	/*
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"info_ladder",

	"keyframe_rope",
	"move_rope",

	"shadow_control",
	"sky_camera",
	
	"trigger_soundscape",
	*/
};

////////////////////////////////////////////////////////////////////////////////
// I'll give you this one rival!
FORCEINLINE bool NamesMatch( const char *pszQuery, string_t nameToMatch )
{
	if ( nameToMatch == NULL_STRING )
		return (*pszQuery == 0 || *pszQuery == '*');

	const char *pszNameToMatch = STRING(nameToMatch);

	// If the pointers are identical, we're identical
	if ( pszNameToMatch == pszQuery )
		return true;

	while ( *pszNameToMatch && *pszQuery )
	{
		char cName = *pszNameToMatch;
		char cQuery = *pszQuery;
		if ( cName != cQuery && tolower(cName) != tolower(cQuery) ) // people almost always use lowercase, so assume that first
			break;
		++pszNameToMatch;
		++pszQuery;
	}

	if ( *pszQuery == 0 && *pszNameToMatch == 0 )
		return true;

	// @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
	if ( *pszQuery == '*' )
		return true;

	return false;
}

bool FindInList(const char **pStrings, const char *pToFind)
{
	int i = 0;
	while(pStrings[i][0] != 0)
	{
		if(Q_stricmp(pStrings[i], pToFind) == 0)
		{
			return true;
		}
		i++;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool LUACheckTransmit(int PlayerIndex, int EntIndex) 
{
	gLua->PushReference(EntityRef);
		gLua->Push((float)PlayerIndex);
	gLua->Call(1, 1);
	ILuaObject *Player = gLua->GetReturn(0);

	gLua->PushReference(EntityRef);
		gLua->Push((float)EntIndex);
	gLua->Call(1, 1);
	ILuaObject *Entity = gLua->GetReturn(0);

	gLua->PushReference(HookCallRef);
		gLua->Push("SameInstance");
		gLua->PushNil();
		gLua->Push(Player);
		gLua->Push(Entity);
	gLua->Call(4, 1);
	ILuaObject *Ret = gLua->GetReturn(0);

	Player->UnReference();
	Entity->UnReference();

	bool Result = Ret->GetBool();
	
	Ret->UnReference();

	return Result;
}

////////////////////////////////////////////////////////////////////////////////

LUA_FUNCTION(PlayerUpdatePVS)
{
	gLua->CheckType(1, GLua::TYPE_ENTITY);

	gLua->PushReference(EntIndexRef);
		gLua->Push(gLua->GetObject(1));
	gLua->Call(1, 1);

	ILuaObject *Ret = gLua->GetReturn(0);

	int EntIndex = Ret->GetInt();

	Ret->UnReference();

	if(EntIndex > 0)
	{
		ClientTransmitInfo[EntIndex].NextUpdate = engine->Time() - 1;
	}

	return 0;
}

LUA_FUNCTION(LUASetUpdateTime)
{
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	UpdateTime = gLua->GetNumber(1);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void UpdatePlayerEdictIndices(int ClientIndex, unsigned short *pEdictIndices, int nEdicts)
{
	ClientTransmitInfo[ClientIndex].nNewEdicts = nEdicts;
	ClientTransmitInfo[ClientIndex].pNewEdictIndices = pEdictIndices;
}

// virtual void CheckTransmit( CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts ) = 0;
DEFVFUNC_(origCheckTransmit, void, (IServerGameEnts *ge, CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts));
void VFUNC newCheckTransmit(IServerGameEnts *ge, CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts)
{
	if(!pBaseEdict || pBaseEdict->IsFree())
	{
		gLua->Error("gm_pvs: The world is missing!\n");
	}

	CBaseEntity *pRecipientEntity = CBaseEntity::Instance(pInfo->m_pClientEnt);
	if(!pRecipientEntity)
	{
		return;
	}

	if(pRecipientEntity->IsPlayer() && pRecipientEntity->GetElasticity() != PVS_CONNECTED)
	{
		return origCheckTransmit(ge, pInfo, pEdictIndices, nEdicts);
	}

	int nNewEdicts = 0;
	int ClientIndex = engine->IndexOfEdict(pInfo->m_pClientEnt);

	if(ClientTransmitInfo[ClientIndex].NextUpdate != NULL)
	{
		if(ClientTransmitInfo[ClientIndex].NextUpdate > engine->Time())
		{
			unsigned short *pNewEdictIndices = new unsigned short[ClientTransmitInfo[ClientIndex].nNewEdicts];

			for(int i=0; i < ClientTransmitInfo[ClientIndex].nNewEdicts; i++)
			{
				int iEdict = ClientTransmitInfo[ClientIndex].pNewEdictIndices[i];
				edict_t *pEdict = &pBaseEdict[iEdict];
				if(pEdict && !pEdict->IsFree() && pEdict->GetUnknown() != NULL)
				{
					CBaseEntity *pEntity = pEdict->GetUnknown()->GetBaseEntity();
					if(pEntity)
					{
						pNewEdictIndices[nNewEdicts] = iEdict;
						nNewEdicts = nNewEdicts + 1;
					}
				}
			}

			UpdatePlayerEdictIndices(ClientIndex, pNewEdictIndices, nNewEdicts);

			return origCheckTransmit(ge, pInfo, pNewEdictIndices, nNewEdicts);
		}
	}

	ClientTransmitInfo[ClientIndex].NextUpdate = engine->Time() + UpdateTime;

	unsigned short *pNewEdictIndices = new unsigned short[nEdicts];

	for(int i=0; i < nEdicts; i++)
	{
		int iEdict = pEdictIndices[i];
		edict_t *pEdict = &pBaseEdict[iEdict];

		if(pEdict && !pEdict->IsFree())
		{
			int nFlags = pEdict->m_fStateFlags & (FL_EDICT_DONTSEND|FL_EDICT_ALWAYS|FL_EDICT_PVSCHECK|FL_EDICT_FULLCHECK);

			if(nFlags & FL_EDICT_DONTSEND)
			{
				continue;
			}

			CBaseEntity *pEntity = pEdict->GetUnknown()->GetBaseEntity();
			if(pEntity)
			{
				string_t ClassName = pEntity->m_iClassname;
				
				int EntIndex = engine->IndexOfEdict(pEdict);

				bool AlwaysInstance = (EntIndex == 0
					|| ClientIndex == EntIndex
					|| FindInList(sAlwaysInstance, ClassName.ToCStr())
					|| NamesMatch("point_*", ClassName)
					|| NamesMatch("func_*", ClassName)
					|| NamesMatch("env_*", ClassName)
					|| NamesMatch("spotlight_*", ClassName));

				if(AlwaysInstance || LUACheckTransmit(ClientIndex, EntIndex) || (pEntity->IsPlayer() && pEntity->GetElasticity() != PVS_CONNECTED))
				{
					//Msg("Manually Set %i:%i\n", nNewEdicts, iEdict);
					pNewEdictIndices[nNewEdicts] = iEdict;
					nNewEdicts = nNewEdicts + 1;
				}

				/*
				if(AlwaysInstance || LUACheckTransmit(PlayerIndex, EntIndex))
				{
					pInfo->m_pTransmitEdict->Set(iEdict);
				}
				else if (pInfo->m_pTransmitEdict->Get(iEdict))
				{
					pInfo->m_pTransmitEdict->Clear(iEdict);
				}*/

			}
		}
	}

	UpdatePlayerEdictIndices(ClientIndex, pNewEdictIndices, nNewEdicts);

	return origCheckTransmit(ge, pInfo, pNewEdictIndices, nNewEdicts);
}

////////////////////////////////////////

/*
	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 ) = 0;
*/
DEFVFUNC_(origEmitSound1, void, (IEngineSound *es, IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, float flAttenuation, int iFlags, int iPitch,
		const Vector *pOrigin, const Vector *pDirection, CUtlVector< Vector >* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity));

void VFUNC newEmitSound1(IEngineSound *es, IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1)
{
	int Count = filter.GetRecipientCount();

	MRecipientFilter MFilter(engine);

	for(int i=0; i < Count; i++)
	{
		int Index = filter.GetRecipientIndex(i);
		if(LUACheckTransmit(iEntIndex, Index))
		{
			MFilter.AddPlayer(Index);
		}
	}

	return origEmitSound1(es, MFilter, iEntIndex, iChannel, pSample, flVolume, flAttenuation, iFlags, iPitch, pOrigin, pDirection,  pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);
}

/*
	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 ) = 0;

DEFVFUNC_(origEmitSound2, void, (IEngineSound *es, IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, soundlevel_t iSoundlevel, int iFlags, int iPitch,
		const Vector *pOrigin, const Vector *pDirection, CUtlVector< Vector >* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity));

void VFUNC newEmitSound2(IEngineSound *es, IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1)
{

	Msg("2\n");
	int Count = filter.GetRecipientCount();

	MRecipientFilter MFilter(engine);

	for(int i=0; i < Count; i++)
	{
		int Index = filter.GetRecipientIndex(i);
		if(LUACheckTransmit(iEntIndex, Index))
		{
			MFilter.AddPlayer(Index);
		}
	}

	return origEmitSound2(es, MFilter, iEntIndex, iChannel, pSample, flVolume, iSoundlevel, iFlags, iPitch, pOrigin, pDirection,  pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);
}
*/

////////////////////////////////////////

int Init(lua_State* L)
{
	gLua = Lua();

	CreateInterfaceFn interfaceFactory = Sys_GetFactory("engine.dll");
	CreateInterfaceFn gameServerFactory = Sys_GetFactory("server.dll");

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if(!engine)
	{
		gLua->Error("gm_pvs: Missing IVEngineServer interface.\n");
	}

	sound = (IEngineSound*)interfaceFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, NULL);
	if(!sound)
	{
		gLua->Error("gm_pvs: Missing IEngineSound interface.\n");
	}

	gameents = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, NULL);
	if(!gameents)
	{
		gLua->Error("gm_pvs: Missing IServerGameEnts interface.\n");
	}

	gLua->SetGlobal("PVS_CONNECTED", PVS_CONNECTED);

	ILuaObject *oEntity = gLua->GetGlobal("Entity");
	oEntity->Push();
	EntityRef = gLua->GetReference(-1, true);
	oEntity->UnReference();

	ILuaObject *ohook = gLua->GetGlobal("hook");
		ILuaObject *oCall = ohook->GetMember("Call");
		oCall->Push();
		HookCallRef = gLua->GetReference(-1, true);
		oCall->UnReference();
	ohook->UnReference();

	ILuaObject *oEntityMeta = gLua->GetMetaTable("Entity", GLua::TYPE_ENTITY);
		ILuaObject *oEntIndex = oEntityMeta->GetMember("EntIndex");
		oEntIndex->Push();
		EntIndexRef = gLua->GetReference(-1, true);
		oEntIndex->UnReference();
	oEntityMeta->UnReference();

	pBaseEdict = engine->PEntityOfEntIndex(0);

	gLua->SetGlobal("pvsSetUpdateTime", LUASetUpdateTime);

	ILuaObject *PlayerMeta = gLua->GetMetaTable("Player", GLua::TYPE_ENTITY);
	if(PlayerMeta)
	{
		PlayerMeta->SetMember("UpdatePVS", PlayerUpdatePVS);
	}
    PlayerMeta->UnReference();

	HOOKVFUNC(sound, 4, origEmitSound1, newEmitSound1);

	//Wasn't called in my tests
	//HOOKVFUNC(sound, 5, origEmitSound2, newEmitSound2);

	HOOKVFUNC(gameents, 6, origCheckTransmit, newCheckTransmit);

	Msg("gm_pvs: Programmed by Spacetech | Entity: %i | hook.Call: %i | EntIndex: %i\n", EntityRef, HookCallRef, EntIndexRef);

	return 0;
}

int Shutdown(lua_State* L)
{
	if(sound)
	{
		Msg("gm_pvs: Unhooking Sound\n");
		UNHOOKVFUNC(sound, 4, origEmitSound1);
		//UNHOOKVFUNC(sound, 5, origEmitSound2);
	}
	if(gameents)
	{
		Msg("gm_pvs: Unhooking PVS\n");
		UNHOOKVFUNC(gameents, 6, origCheckTransmit);
	}
	return 0;
}
