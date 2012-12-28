/*
    gm_navigation
    By Spacetech
*/

#include <interface.h>
#include "eiface.h"
#include "filesystem.h"
#include "engine/ienginetrace.h"
#include "defines.h"

#ifdef USE_BOOST_THREADS
#include <boost/thread/thread.hpp>
#else
#include <jobthread.h>
IThreadPool* threadPool;
#endif

//#ifdef FILEBUG
	FileHandle_t fh = FILESYSTEM_INVALID_HANDLE;
	FILE *pDebugFile = NULL;
//#endif

IVEngineServer *engine = NULL;
IFileSystem *filesystem = NULL;
IEngineTrace *enginetrace = NULL;
