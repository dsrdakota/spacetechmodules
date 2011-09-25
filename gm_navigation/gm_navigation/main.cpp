/*
    gm_navigation
    By Spacetech
*/

#include "main.h"

GMOD_MODULE(Init, Shutdown);

GMUtility *util = NULL;
IVEngineServer *engine = NULL;
IFileSystem *Gfilesystem = NULL;
IEngineTrace *Genginetrace[2] = {NULL, NULL};

int VectorMetaRef = NULL;

///////////////////////////////////////////////
// @Jinto (Referenced by Spacetech)

ILuaObject* NewVectorObject(lua_State* L, Vector& vec)
{
	if(VectorMetaRef == NULL)
	{
		// @azuisleet Get a reference to the function to survive past 510 calls!
		ILuaObject *VectorMeta = Lua()->GetGlobal("Vector");
			VectorMeta->Push();
			VectorMetaRef = Lua()->GetReference(-1, true);
		VectorMeta->UnReference();
	}

	Lua()->PushReference(VectorMetaRef);
	
	if(Lua()->GetType(-1) != GLua::TYPE_FUNCTION)
	{
		Msg("gm_navigation error: Not a function: %i\n", Lua()->GetType(-1));
		Lua()->Push(Lua()->GetGlobal("Vector"));
	}

		Lua()->Push(vec.x);
		Lua()->Push(vec.y);
		Lua()->Push(vec.z);
	Lua()->Call(3, 1);

	return Lua()->GetReturn(0);
}

void GMOD_PushVector(lua_State* L, Vector& vec)
{
	ILuaObject* obj = NewVectorObject(L, vec);
		Lua()->Push(obj);
	obj->UnReference();
}

Vector& GMOD_GetVector(lua_State* L, int stackPos)
{
	return *reinterpret_cast<Vector*>(Lua()->GetUserData(stackPos));
}

///////////////////////////////////////////////

Nav* GetNav(lua_State* L, int Pos)
{
	Nav* nav = (Nav*)Lua()->GetUserData(Pos);
	if(nav == NULL)
	{
		Lua()->Error("Invalid Nav");
	}
	return nav;
}

Node* GetNode(lua_State* L, int Pos)
{
	return (Node*)Lua()->GetUserData(Pos);
}

void PushNav(lua_State* L, Nav *nav)
{
	if(nav)
	{
		ILuaObject* meta = Lua()->GetMetaTable(NAV_NAME, NAV_TYPE);
			Lua()->PushUserData(meta, nav);
		meta->UnReference();
	}
	else
	{
		Lua()->Push(false);
	}
}

void PushNode(lua_State* L, Node *node)
{
	if(node)
	{
		ILuaObject* meta = Lua()->GetMetaTable(NODE_NAME, NODE_TYPE);
			Lua()->PushUserData(meta, node);
		meta->UnReference();
	}
	else
	{
		Lua()->Push(false);
	}
}

///////////////////////////////////////////////

LUA_FUNCTION(CreateNav)
{
	Lua()->CheckType(1, GLua::TYPE_NUMBER);

	PushNav(L, new Nav(util, Gfilesystem, (int)Lua()->GetNumber(1)));

	return 1;
}

///////////////////////////////////////////////

LUA_FUNCTION(Nav_GetNodeByID)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	PushNode(L, GetNav(L, 1)->GetNodeByID((int)Lua()->GetNumber(2) - 1));

	return 1;
}

LUA_FUNCTION(Nav_GetNodeTotal)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push((float)GetNav(L, 1)->GetNodes().Count());

	return 1;
}

LUA_FUNCTION(Nav_GetNodes)
{
	Lua()->CheckType(1, NAV_TYPE);

	NodeList_t& Nodes = GetNav(L, 1)->GetNodes();

	ILuaObject *NodeTable = Lua()->GetNewTable();
	NodeTable->Push();
	NodeTable->UnReference();

	for(int i = 0; i < Nodes.Count(); i++)
	{
		PushNode(L, Nodes[i]);
		ILuaObject *ObjNode = Lua()->GetObject();
		NodeTable = Lua()->GetObject(2);
			NodeTable->SetMember((float)i + 1, ObjNode);
			Lua()->Pop();
		NodeTable->UnReference();
		ObjNode->UnReference();
	}

	return 1;
}

LUA_FUNCTION(Nav_StartGeneration)
{
	Lua()->CheckType(1, NAV_TYPE);

	GetNav(L, 1)->StartGeneration();

	return 0;
}

LUA_FUNCTION(Nav_SetupMaxDistance)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_NUMBER);

	GetNav(L, 1)->SetupMaxDistance(GMOD_GetVector(L, 2), Lua()->GetNumber(3));

	return 0;
}

LUA_FUNCTION(Nav_AddWalkableSeed)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_VECTOR);

	GetNav(L, 1)->AddWalkableSeed(GMOD_GetVector(L, 2), GMOD_GetVector(L, 3));

	return 0;
}

LUA_FUNCTION(Nav_ClearWalkableSeeds)
{
	Lua()->CheckType(1, NAV_TYPE);

	GetNav(L, 1)->ClearWalkableSeeds();

	return 0;
}


LUA_FUNCTION(Nav_UpdateGeneration)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push(GetNav(L, 1)->SampleStep());

	return 1;
}

LUA_FUNCTION(Nav_FullGeneration)
{
	Lua()->CheckType(1, NAV_TYPE);

	Nav *nav = GetNav(L, 1);

	float StartTime = engine->Time();

	while(!nav->SampleStep())
	{
	}

	Lua()->Push(engine->Time() - StartTime);

	return 1;
}

LUA_FUNCTION(Nav_IsGenerated)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push(GetNav(L, 1)->IsGenerated());

	return 1;
}

LUA_FUNCTION(Nav_FindPath)
{
	Lua()->CheckType(1, NAV_TYPE);

	Nav *nav = GetNav(L, 1);

	NodeList_t& Path = nav->FindPath();

	if(nav->HasFoundPath())
	{
		Lua()->Push(true);
	}
	else
	{
		Lua()->Push(false);
	}

	ILuaObject *PathTable = Lua()->GetNewTable();
	PathTable->Push();
	PathTable->UnReference();

	for(int i = 0; i < Path.Count(); i++)
	{
		PushNode(L, Path[i]);
		ILuaObject *ObjNode = Lua()->GetObject();
		PathTable = Lua()->GetObject(3); // It's 3 since we pushed a bool before the table
			PathTable->SetMember((float)i + 1, ObjNode);
			Lua()->Pop();
		PathTable->UnReference();
		ObjNode->UnReference();
	}

	return 2;
}

LUA_FUNCTION(Nav_GetHeuristic)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push((float)GetNav(L, 1)->GetHeuristic());

	return 1;
}

LUA_FUNCTION(Nav_GetStart)
{
	Lua()->CheckType(1, NAV_TYPE);

	PushNode(L, GetNav(L, 1)->GetStart());

	return 1;
}

LUA_FUNCTION(Nav_GetEnd)
{
	Lua()->CheckType(1, NAV_TYPE);

	PushNode(L, GetNav(L, 1)->GetEnd());

	return 1;
}

LUA_FUNCTION(Nav_SetHeuristic)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	GetNav(L, 1)->SetHeuristic(Lua()->GetNumber(2));

	return 0;
}

LUA_FUNCTION(Nav_SetStart)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	GetNav(L, 1)->SetStart(GetNode(L, 2));

	return 0;
}

LUA_FUNCTION(Nav_SetEnd)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	GetNav(L, 1)->SetEnd(GetNode(L, 2));

	return 0;
}

LUA_FUNCTION(Nav_GetNode)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	PushNode(L, GetNav(L, 1)->GetNode(GMOD_GetVector(L, 2)));

	return 1;
}

LUA_FUNCTION(Nav_GetClosestNode)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	PushNode(L, GetNav(L, 1)->GetClosestNode(GMOD_GetVector(L, 2)));

	return 1;
}

LUA_FUNCTION(Nav_Save)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_STRING);

	Lua()->Push(GetNav(L, 1)->Save(Lua()->GetString(2)));

	return 1;
}

LUA_FUNCTION(Nav_Load)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_STRING);

	Lua()->Push(GetNav(L, 1)->Load(Lua()->GetString(2)));

	return 1;
}

LUA_FUNCTION(Nav_GetDiagonal)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push(GetNav(L, 1)->GetDiagonal());

	return 1;
}

LUA_FUNCTION(Nav_SetDiagonal)
{
	Lua()->CheckType(1, NAV_TYPE);

	GetNav(L, 1)->SetDiagonal(Lua()->GetBool(2));

	return 0;
}

LUA_FUNCTION(Nav_GetGridSize)
{
	Lua()->CheckType(1, NAV_TYPE);
	
	Lua()->Push((float)GetNav(L, 1)->GetGridSize());

	return 1;
}

LUA_FUNCTION(Nav_SetGridSize)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	GetNav(L, 1)->SetGridSize(Lua()->GetInteger(2));

	return 0;
}

LUA_FUNCTION(Nav_GetMask)
{
	Lua()->CheckType(1, NAV_TYPE);
	
	Lua()->Push((float)GetNav(L, 1)->GetMask());

	return 1;
}

LUA_FUNCTION(Nav_SetMask)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	GetNav(L, 1)->SetMask(Lua()->GetInteger(2));

	return 0;
}

LUA_FUNCTION(Nav_CreateNode)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_VECTOR);

	PushNode(L, GetNav(L, 1)->AddNode(GMOD_GetVector(L, 2), GMOD_GetVector(L, 3), NORTH, NULL)); // This dir doesn't matter

	return 1;
}

LUA_FUNCTION(Nav_RemoveNode)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	GetNav(L, 1)->RemoveNode(GetNode(L, 2));

	return 0;
}

///////////////////////////////////////////////

LUA_FUNCTION(Node__eq)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	Lua()->Push((bool)(GetNode(L, 1)->GetID() == GetNode(L, 2)->GetID()));

	return 1;
}

LUA_FUNCTION(Node_GetID)
{
	Lua()->CheckType(1, NODE_TYPE);

	Lua()->Push((float)GetNode(L, 1)->GetID() + 1);

	return 1;
}

LUA_FUNCTION(Node_GetPosition)
{
	Lua()->CheckType(1, NODE_TYPE);

	GMOD_PushVector(L, GetNode(L, 1)->Pos);

	return 1;
}

LUA_FUNCTION(Node_GetNormal)
{
	Lua()->CheckType(1, NODE_TYPE);

	GMOD_PushVector(L, GetNode(L, 1)->Normal);

	return 1;
}

LUA_FUNCTION(Node_IsDisabled)
{
	Lua()->CheckType(1, NODE_TYPE);

	Lua()->Push(GetNode(L, 1)->IsDisabled());

	return 1;
}

LUA_FUNCTION(Node_SetDisabled)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_BOOL);

	GetNode(L, 1)->SetDisabled(Lua()->GetBool(2));

	return 0;

}

LUA_FUNCTION(Node_GetConnections)
{
	Lua()->CheckType(1, NODE_TYPE);

	Node *node = GetNode(L, 1);

	// Push the table on the stack to survive past 510 calls!
	ILuaObject *NodeTable = Lua()->GetNewTable();
	NodeTable->Push();
	NodeTable->UnReference();
	
	Node *Connection;

	for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
	{
		Connection = node->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL)
		{
			PushNode(L, Connection);
			ILuaObject *ObjNode = Lua()->GetObject();
			NodeTable = Lua()->GetObject(2);
				NodeTable->SetMember((float)Dir, ObjNode);
				Lua()->Pop();
			NodeTable->UnReference();
			ObjNode->UnReference();
		}
	}

	return 1;
}

LUA_FUNCTION(Node_IsConnected)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	Node *Node1 = GetNode(L, 1);
	Node *Node2 = GetNode(L, 2);

	Node *Connection;
	for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
	{
		Connection = Node1->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL && Connection == Node2)
		{
			Lua()->Push(true);

			return 1;
		}
	}

	Lua()->Push(false);

	return 1;
}

LUA_FUNCTION(Node_SetNormal)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	GetNode(L, 1)->SetNormal(GMOD_GetVector(L, 2));

	return 0;
}

LUA_FUNCTION(Node_SetPosition)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	GetNode(L, 1)->SetPosition(GMOD_GetVector(L, 2));

	return 0;
}

LUA_FUNCTION(Node_ConnectTo)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, NODE_TYPE);
	Lua()->CheckType(3, GLua::TYPE_NUMBER);

	Node *Node1 = GetNode(L, 1);
	Node *Node2 = GetNode(L, 2);
	NavDirType Dir = (NavDirType)Lua()->GetInteger(3);

	Node1->ConnectTo(Node2, Dir);
	Node2->ConnectTo(Node1, OppositeDirection(Dir));

	Node1->MarkAsVisited(Dir);
	Node1->MarkAsVisited(OppositeDirection(Dir));

	Node2->MarkAsVisited(Dir);

	return 0;
}

LUA_FUNCTION(Node_RemoveConnection)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	Node *Node1 = GetNode(L, 1);
	NavDirType Dir = (NavDirType)Lua()->GetInteger(2);

	Node *Node2 = Node1->GetConnectedNode(Dir);
	if(Node2 != NULL)
	{
		Node1->ConnectTo(NULL, Dir);
		Node2->ConnectTo(NULL, OppositeDirection(Dir));

		// UnMarkAsVisited?
		// I don't really know bitwise too well
	}

	return 0;
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

factorylist_t GetFactories(lua_State* L)
{
	CSigScan::sigscan_dllfunc = Sys_GetFactory("server.dll");

	if(!CSigScan::GetDllMemInfo())
	{
		Lua()->Error("Could not get access to factories.");
	}

	CSigScan sigscanFactories;
	sigscanFactories.Init((unsigned char *)"\x8B\x44\x24\x04\x8B\x0D\xC0\x33\x6D\x10\x8B\x15\xC4\x33\x6D\x10\x89\x08\x8B\x0D\xC8\x33\x6D\x10\x89\x50\x04\x89\x48\x08\xC3", "xxxxxx????xx????xxxx????xxxxxxx", 31);

	if(!sigscanFactories.is_set)
	{
		Lua()->Error("Could not get access to factories.");
	}

	FactoryList_Retrieve = (void (*)(factorylist_t &))sigscanFactories.sig_addr;

	factorylist_t factories;
	FactoryList_Retrieve(factories);

	return factories;
}

///////////////////////////////////////////////

int Init(lua_State* L)
{
	CreateInterfaceFn interfaceFactory = Sys_GetFactory("engine.dll");

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if(!engine)
	{
		Lua()->Error("gm_navigation: Missing IVEngineServer interface.\n");
	}

	if(Lua()->IsClient())
	{
		Genginetrace[1] = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_CLIENT, NULL);
	}
	else
	{
		Genginetrace[0] = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL);
	}
	
	if(!Genginetrace[(int)Lua()->IsClient()])
	{
		Lua()->Error("gm_navigation: Missing IEngineTrace interface.\n");
	}

	CreateInterfaceFn fsFactory;

	// This check probably isn't needed
	if(Lua()->IsDedicatedServer())
	{
		fsFactory = GetFactories(L).fileSystemFactory;
	}
	else
	{
		fsFactory = Sys_GetFactory("filesystem_steam.dll");
	}

	if(!fsFactory)
	{
		Lua()->Error("gm_navigation: Missing fsFactory\n");
	}

	Gfilesystem = (IFileSystem*)fsFactory(FILESYSTEM_INTERFACE_VERSION, NULL);
	if(!Gfilesystem)
	{
		Lua()->Error("gm_navigation: Missing IFileSystem interface.\n");
	}

	util = new GMUtility(Genginetrace[(int)Lua()->IsClient()], Lua()->IsServer());

	Lua()->SetGlobal("NORTH", (float)NORTH);
	Lua()->SetGlobal("SOUTH", (float)SOUTH);
	Lua()->SetGlobal("EAST", (float)EAST);
	Lua()->SetGlobal("WEST", (float)WEST);

	Lua()->SetGlobal("NORTHEAST", (float)NORTHEAST);
	Lua()->SetGlobal("NORTHWEST", (float)NORTHWEST);
	Lua()->SetGlobal("SOUTHEAST", (float)SOUTHEAST);
	Lua()->SetGlobal("SOUTHWEST", (float)SOUTHWEST);

	Lua()->SetGlobal("NUM_DIRECTIONS", (float)NUM_DIRECTIONS);
	Lua()->SetGlobal("NUM_DIRECTIONS_DIAGONAL", (float)NUM_DIRECTIONS_DIAGONAL);
	Lua()->SetGlobal("NUM_DIRECTIONS_MAX", (float)NUM_DIRECTIONS_MAX);

	Lua()->SetGlobal("HEURISTIC_MANHATTAN", (float)Nav::HEURISTIC_MANHATTAN);
	Lua()->SetGlobal("HEURISTIC_EUCLIDEAN", (float)Nav::HEURISTIC_EUCLIDEAN);
	Lua()->SetGlobal("HEURISTIC_CUSTOM", (float)Nav::HEURISTIC_CUSTOM);

	Lua()->SetGlobal("CreateNav", CreateNav);

	ILuaObject *MetaNav = Lua()->GetMetaTable(NAV_NAME, NAV_TYPE);
		ILuaObject *NavIndex = Lua()->GetNewTable();
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
			NavIndex->SetMember("GetDiagonal", Nav_GetDiagonal);
			NavIndex->SetMember("SetDiagonal", Nav_SetDiagonal);
			NavIndex->SetMember("GetGridSize", Nav_GetGridSize);
			NavIndex->SetMember("SetGridSize", Nav_SetGridSize);
			NavIndex->SetMember("GetMask", Nav_GetMask);
			NavIndex->SetMember("SetMask", Nav_SetMask);
			NavIndex->SetMember("CreateNode", Nav_CreateNode);
			NavIndex->SetMember("RemoveNode", Nav_RemoveNode);

		MetaNav->SetMember("__index", NavIndex);
		NavIndex->UnReference();
	MetaNav->UnReference();

	ILuaObject *MetaNode = Lua()->GetMetaTable(NODE_NAME, NODE_TYPE);
		ILuaObject *NodeIndex = Lua()->GetNewTable();
			NodeIndex->SetMember("GetID", Node_GetID);
			
			NodeIndex->SetMember("GetPosition", Node_GetPosition);
			NodeIndex->SetMember("GetNormal", Node_GetNormal);
			NodeIndex->SetMember("GetConnections", Node_GetConnections);
			NodeIndex->SetMember("IsConnected", Node_IsConnected);

			NodeIndex->SetMember("IsDisabled", Node_IsDisabled);
			NodeIndex->SetMember("SetDisabled", Node_SetDisabled);

			NodeIndex->SetMember("SetNormal", Node_SetNormal);
			NodeIndex->SetMember("SetPosition", Node_SetPosition);
			NodeIndex->SetMember("ConnectTo", Node_ConnectTo);
			NodeIndex->SetMember("RemoveConnection", Node_RemoveConnection);

		MetaNode->SetMember("__index", NodeIndex);
		MetaNode->SetMember("__eq", Node__eq);

		NodeIndex->UnReference();
	MetaNode->UnReference();

	Msg("gm_navigation: Programmed by Spacetech\n");

	return 0;
}

int Shutdown(lua_State* L)
{
	if(VectorMetaRef)
	{
		Lua()->FreeReference(VectorMetaRef);
		VectorMetaRef = NULL;
	}
	return 0;
}
