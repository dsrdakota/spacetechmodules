/*
    gm_navigation
    By Spacetech
*/

#include <interface.h>
#include "eiface.h"
#include "filesystem.h"
#include "engine/ienginetrace.h"
#include "tier0/memdbgon.h"

#include "gmluamodule.h"

#include "sigscan.h"

#define NAV_NAME "Nav"
#define NAV_TYPE 7456

#define NODE_NAME "Node"
#define NODE_TYPE 7457

#include "gmutility.h"

#include "node.h"
#include "nav.h"

typedef CUtlVector<Node*> NodeList_t;

NavDirType OppositeDirection(NavDirType Dir)
{
	switch(Dir)
	{
		case NORTH: return SOUTH;
		case SOUTH: return NORTH;
		case EAST:	return WEST;
		case WEST:	return EAST;
	}
	switch(Dir)
	{
		case NORTHEAST: return SOUTHWEST;
		case NORTHWEST: return SOUTHEAST;
		case SOUTHEAST:	return NORTHWEST;
		case SOUTHWEST: return NORTHEAST;
	}
	return NORTH;
}
