/*
    gm_navigation
    By Spacetech
*/

#include "main.h"

GMOD_MODULE(Init, Shutdown);

GMUtility *util = NULL;
ILuaInterface *gLua = NULL;
IVEngineServer *engine = NULL;
IFileSystem *Gfilesystem = NULL;
IEngineTrace *Genginetrace = NULL;

int VectorMetaRef = -1;

///////////////////////////////////////////////
// @Jinto (Referenced by Spacetech)

ILuaObject* NewVectorObject(Vector& vec)
{
	gLua->PushReference(VectorMetaRef);
		gLua->Push(vec.x);
		gLua->Push(vec.y);
		gLua->Push(vec.z);
	gLua->Call(3, 1);

	return gLua->GetReturn(0);
}

void GMOD_PushVector(Vector& vec)
{
	ILuaObject* obj = NewVectorObject(vec);
		gLua->Push(obj);
	obj->UnReference();
}

Vector& GMOD_GetVector(int stackPos)
{
	return *reinterpret_cast<Vector*>(gLua->GetUserData(stackPos));
}

///////////////////////////////////////////////

Nav* GetNav(int Pos)
{
	return (Nav*)gLua->GetUserData(Pos);
}

Node* GetNode(int Pos)
{
	return (Node*)gLua->GetUserData(Pos);
}

void PushNav(Nav *nav)
{
	if(nav)
	{
		ILuaObject* meta = gLua->GetMetaTable(NAV_NAME, NAV_TYPE);
			gLua->PushUserData(meta, nav);
		meta->UnReference();
	}
	else
	{
		gLua->Push(false);
	}
}

void PushNode(Node *node)
{
	if(node)
	{
		ILuaObject* meta = gLua->GetMetaTable(NODE_NAME, NODE_TYPE);
			gLua->PushUserData(meta, node);
		meta->UnReference();
	}
	else
	{
		gLua->Push(false);
	}
}

///////////////////////////////////////////////

LUA_FUNCTION(CreateNav)
{
	gLua->CheckType(1, GLua::TYPE_NUMBER);
	PushNav(new Nav(util, Gfilesystem, (int)gLua->GetNumber(1)));
	return 1;
}

///////////////////////////////////////////////

LUA_FUNCTION(Nav_GetNodeByID)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	PushNode(GetNav(1)->GetNodeByID((int)gLua->GetNumber(2) - 1));

	return 1;
}

LUA_FUNCTION(Nav_GetNodeTotal)
{
	gLua->CheckType(1, NAV_TYPE);

	gLua->Push((float)GetNav(1)->GetNodes().Count());

	return 1;
}

LUA_FUNCTION(Nav_GetNodes)
{
	gLua->CheckType(1, NAV_TYPE);

	NodeList_t& Nodes = GetNav(1)->GetNodes();

	ILuaObject *NodeTable = gLua->GetNewTable();
	NodeTable->Push();
	NodeTable->UnReference();

	for(int i = 0; i < Nodes.Count(); i++)
	{
		PushNode(Nodes[i]);
		ILuaObject *ObjNode = gLua->GetObject();
		NodeTable = gLua->GetObject(2);
			NodeTable->SetMember((float)i + 1, ObjNode);
			gLua->Pop();
		NodeTable->UnReference();
		ObjNode->UnReference();
	}

	return 1;
}

LUA_FUNCTION(Nav_StartGeneration)
{
	gLua->CheckType(1, NAV_TYPE);

	GetNav(1)->StartGeneration();

	return 0;
}

LUA_FUNCTION(Nav_SetupMaxDistance)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, GLua::TYPE_VECTOR);
	gLua->CheckType(3, GLua::TYPE_NUMBER);

	GetNav(1)->SetupMaxDistance(GMOD_GetVector(2), gLua->GetNumber(3));

	return 0;
}

LUA_FUNCTION(Nav_AddWalkableSeed)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, GLua::TYPE_VECTOR);
	gLua->CheckType(3, GLua::TYPE_VECTOR);

	GetNav(1)->AddWalkableSeed(GMOD_GetVector(2), GMOD_GetVector(3));

	return 0;
}

LUA_FUNCTION(Nav_ClearWalkableSeeds)
{
	gLua->CheckType(1, NAV_TYPE);

	GetNav(1)->ClearWalkableSeeds();

	return 0;
}


LUA_FUNCTION(Nav_UpdateGeneration)
{
	gLua->CheckType(1, NAV_TYPE);

	gLua->Push(GetNav(1)->SampleStep());

	return 1;
}

LUA_FUNCTION(Nav_FullGeneration)
{
	gLua->CheckType(1, NAV_TYPE);

	Nav *nav = GetNav(1);

	float StartTime = engine->Time();

	while(!nav->SampleStep())
	{
	}

	gLua->Push(engine->Time() - StartTime);

	return 1;
}

LUA_FUNCTION(Nav_IsGenerated)
{
	gLua->CheckType(1, NAV_TYPE);

	gLua->Push(GetNav(1)->IsGenerated());

	return 1;
}

LUA_FUNCTION(Nav_FindPath)
{
	gLua->CheckType(1, NAV_TYPE);

	Nav *nav = GetNav(1);

	NodeList_t& Path = nav->FindPath();

	if(nav->HasFoundPath())
	{
		gLua->Push(true);
	}
	else
	{
		gLua->Push(false);
	}

	ILuaObject *PathTable = gLua->GetNewTable();
	PathTable->Push();
	PathTable->UnReference();

	for(int i = 0; i < Path.Count(); i++)
	{
		PushNode(Path[i]);
		ILuaObject *ObjNode = gLua->GetObject();
		PathTable = gLua->GetObject(3); // It's 3 since we pushed a bool before the table
			PathTable->SetMember((float)i + 1, ObjNode);
			gLua->Pop();
		PathTable->UnReference();
		ObjNode->UnReference();
	}

	return 2;
}

LUA_FUNCTION(Nav_GetHeuristic)
{
	gLua->CheckType(1, NAV_TYPE);

	gLua->Push((float)GetNav(1)->GetHeuristic());

	return 1;
}

LUA_FUNCTION(Nav_GetStart)
{
	gLua->CheckType(1, NAV_TYPE);

	PushNode(GetNav(1)->GetStart());

	return 1;
}

LUA_FUNCTION(Nav_GetEnd)
{
	gLua->CheckType(1, NAV_TYPE);

	PushNode(GetNav(1)->GetEnd());

	return 1;
}

LUA_FUNCTION(Nav_SetHeuristic)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	GetNav(1)->SetHeuristic(gLua->GetNumber(2));

	return 0;
}

LUA_FUNCTION(Nav_SetStart)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, NODE_TYPE);

	GetNav(1)->SetStart(GetNode(2));

	return 0;
}

LUA_FUNCTION(Nav_SetEnd)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, NODE_TYPE);

	GetNav(1)->SetEnd(GetNode(2));

	return 0;
}

LUA_FUNCTION(Nav_GetNode)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, GLua::TYPE_VECTOR);

	PushNode(GetNav(1)->GetNode(GMOD_GetVector(2)));

	return 1;
}

LUA_FUNCTION(Nav_GetClosestNode)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, GLua::TYPE_VECTOR);

	PushNode(GetNav(1)->GetClosestNode(GMOD_GetVector(2)));

	return 1;
}

LUA_FUNCTION(Nav_Save)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, GLua::TYPE_STRING);

	gLua->Push(GetNav(1)->Save(gLua->GetString(2)));

	return 1;
}

LUA_FUNCTION(Nav_Load)
{
	gLua->CheckType(1, NAV_TYPE);
	gLua->CheckType(2, GLua::TYPE_STRING);

	gLua->Push(GetNav(1)->Load(gLua->GetString(2)));

	return 1;
}
///////////////////////////////////////////////

LUA_FUNCTION(Node_GetPosition)
{
	gLua->CheckType(1, NODE_TYPE);

	GMOD_PushVector(GetNode(1)->Pos);

	return 1;
}

LUA_FUNCTION(Node_GetNormal)
{
	gLua->CheckType(1, NODE_TYPE);

	GMOD_PushVector(GetNode(1)->Normal);

	return 1;
}

LUA_FUNCTION(Node_GetConnections)
{
	gLua->CheckType(1, NODE_TYPE);

	Node *node = GetNode(1);

	// Push the table on the stack to survive past 510 calls!
	ILuaObject *NodeTable = gLua->GetNewTable();
	NodeTable->Push();
	NodeTable->UnReference();
	
	Node *Connection;

	for(int Dir = NORTH; Dir < NUM_DIRECTIONS; Dir++)
	{
		Connection = node->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL)
		{
			PushNode(Connection);
			ILuaObject *ObjNode = gLua->GetObject();
			NodeTable = gLua->GetObject(2);
				NodeTable->SetMember((float)Dir, ObjNode);
				gLua->Pop();
			NodeTable->UnReference();
			ObjNode->UnReference();
		}
	}

	return 1;
}

LUA_FUNCTION(Node_IsConnected)
{
	gLua->CheckType(1, NODE_TYPE);
	gLua->CheckType(2, NODE_TYPE);

	Node *Node1 = GetNode(1);
	Node *Node2 = GetNode(2);

	Node *Connection;
	for(int Dir = NORTH; Dir < NUM_DIRECTIONS; Dir++)
	{
		Connection = Node1->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL && Connection == Node2)
		{
			gLua->Push(true);

			return 1;
		}
	}

	gLua->Push(false);

	return 1;
}

///////////////////////////////////////////////
// @haza55

struct factorylist_t
{
	CreateInterfaceFn engineFactory;
	CreateInterfaceFn physicsFactory;
	CreateInterfaceFn fileSystemFactory;
};

void (*FactoryList_Retrieve)( factorylist_t &destData );

factorylist_t GetFactories()
{
	CSigScan::sigscan_dllfunc = Sys_GetFactory("server.dll");

	if(!CSigScan::GetDllMemInfo())
	{
		gLua->Error("Could not get access to factories.");
	}

	CSigScan sigscanFactories;
	sigscanFactories.Init((unsigned char *)"\x8B\x44\x24\x04\x8B\x0D\xC0\x33\x6D\x10\x8B\x15\xC4\x33\x6D\x10\x89\x08\x8B\x0D\xC8\x33\x6D\x10\x89\x50\x04\x89\x48\x08\xC3", "xxxxxx????xx????xxxx????xxxxxxx", 31);

	if(!sigscanFactories.is_set)
	{
		gLua->Error("Could not get access to factories.");
	}

	FactoryList_Retrieve = (void (*)(factorylist_t &))sigscanFactories.sig_addr;

	factorylist_t factories;
	FactoryList_Retrieve(factories);

	return factories;
}

///////////////////////////////////////////////


int Init(lua_State* L)
{
	gLua = Lua();

	CreateInterfaceFn interfaceFactory = Sys_GetFactory("engine.dll");

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if(!engine)
	{
		gLua->Error("gm_nav: Missing IVEngineServer interface.\n");
	}

	if(gLua->IsServer())
	{
		Genginetrace = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL);
	}
	else
	{
		Genginetrace = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_CLIENT, NULL);
	}
	
	if(!Genginetrace)
	{
		gLua->Error("gm_navigation: Missing IEngineTrace interface.\n");
	}

	CreateInterfaceFn fsFactory;

	if(gLua->IsDedicatedServer())
	{
		fsFactory = GetFactories().fileSystemFactory;
	}
	else
	{
		fsFactory = Sys_GetFactory("filesystem_steam.dll");
	}

	if(!fsFactory)
	{
		gLua->Error("gm_navigation: Missing fsFactory\n");
	}

	Gfilesystem = (IFileSystem*)fsFactory(FILESYSTEM_INTERFACE_VERSION, NULL);
	if(!Gfilesystem)
	{
		gLua->Error("gm_navigation: Missing IFileSystem interface.\n");
	}

	// @azuisleet Get a reference to the function to survive past 510 calls!
	ILuaObject *VectorMeta = gLua->GetGlobal("Vector");
		VectorMeta->Push();
		VectorMetaRef = gLua->GetReference(-1, true);
	VectorMeta->UnReference();

	util = new GMUtility(Genginetrace, gLua->IsServer());

#ifdef DIAGONAL
	gLua->SetGlobal("DIAGONAL", true);
#endif

	gLua->SetGlobal("NORTH", (float)NORTH);
	gLua->SetGlobal("SOUTH", (float)SOUTH);
	gLua->SetGlobal("EAST", (float)EAST);
	gLua->SetGlobal("WEST", (float)WEST);

#ifdef DIAGONAL
	gLua->SetGlobal("NORTHEAST", (float)NORTHEAST);
	gLua->SetGlobal("NORTHWEST", (float)NORTHWEST);
	gLua->SetGlobal("SOUTHEAST", (float)SOUTHEAST);
	gLua->SetGlobal("SOUTHWEST", (float)SOUTHWEST);
#endif

	gLua->SetGlobal("NUM_DIRECTIONS", (float)NUM_DIRECTIONS);

	gLua->SetGlobal("HEURISTIC_MANHATTAN", (float)Nav::HEURISTIC_MANHATTAN);
	gLua->SetGlobal("HEURISTIC_EUCLIDEAN", (float)Nav::HEURISTIC_EUCLIDEAN);
	gLua->SetGlobal("HEURISTIC_CUSTOM", (float)Nav::HEURISTIC_CUSTOM);

	gLua->SetGlobal("CreateNav", CreateNav);

	ILuaObject *MetaNav = gLua->GetMetaTable(NAV_NAME, NAV_TYPE);
		ILuaObject *NavIndex = gLua->GetNewTable();
			NavIndex->SetMember("GetNodeByID", Nav_GetNodeByID);
			NavIndex->SetMember("GetNodes", Nav_GetNodes);
			NavIndex->SetMember("GetNodeTotal", Nav_GetNodeTotal);
			NavIndex->SetMember("StartGeneration", Nav_StartGeneration);
			NavIndex->SetMember("AddWalkableSeed", Nav_AddWalkableSeed);
			NavIndex->SetMember("ClearWalkableSeeds", Nav_ClearWalkableSeeds);
			NavIndex->SetMember("SetupMaxDistance", Nav_SetupMaxDistance);
			NavIndex->SetMember("UpdateGeneration", Nav_UpdateGeneration);
			NavIndex->SetMember("FullGeneration", Nav_FullGeneration);
			NavIndex->SetMember("IsGenerated", Nav_IsGenerated);
			NavIndex->SetMember("FindPath", Nav_FindPath);
			NavIndex->SetMember("GetHeuristic", Nav_GetHeuristic);
			NavIndex->SetMember("GetStart", Nav_GetStart);
			NavIndex->SetMember("GetEnd", Nav_GetEnd);
			NavIndex->SetMember("SetHeuristic", Nav_SetHeuristic);
			NavIndex->SetMember("SetStart", Nav_SetStart);
			NavIndex->SetMember("SetEnd", Nav_SetEnd);
			NavIndex->SetMember("GetNode", Nav_GetNode);
			NavIndex->SetMember("GetClosestNode", Nav_GetClosestNode);
			NavIndex->SetMember("Save", Nav_Save);
			NavIndex->SetMember("Load", Nav_Load);

		MetaNav->SetMember("__index", NavIndex);
		NavIndex->UnReference();
	MetaNav->UnReference();

	ILuaObject *MetaNode = gLua->GetMetaTable(NODE_NAME, NODE_TYPE);
		ILuaObject *NodeIndex = gLua->GetNewTable();
			NodeIndex->SetMember("GetPosition", Node_GetPosition);
			NodeIndex->SetMember("GetNormal", Node_GetNormal);
			NodeIndex->SetMember("GetConnections", Node_GetConnections);
			NodeIndex->SetMember("IsConnected", Node_IsConnected);
		MetaNode->SetMember("__index", NodeIndex);
		NodeIndex->UnReference();
	MetaNode->UnReference();

	Msg("gm_navigation: Programmed by Spacetech\n");

	return 0;
}

int Shutdown(lua_State* L)
{
	if(VectorMetaRef)
	{
		gLua->FreeReference(VectorMetaRef);
	}
	return 0;
}
