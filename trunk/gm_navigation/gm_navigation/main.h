/*
    gm_navigation
    By Spacetech
*/

#include <interface.h>
#include "eiface.h"
#include "filesystem.h"
#include "engine/ienginetrace.h"
#include "tier0/memdbgon.h"

#include "sigscan.h"

#include "node.h"
#include "nav.h"

#include "gmutility.h"

typedef CUtlVector<Node*> NodeList_t;

CUtlVector<JobInfo_t*> JobQueue;

IThreadPool* threadPool;

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
