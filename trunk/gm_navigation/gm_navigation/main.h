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
