/*
    gm_navigation
    By Spacetech
*/

#include "gmutility.h"

GMUtility::GMUtility(IEngineTrace *et, bool ServerSide)
{
	enginetrace = et;
	traceFilter = new GMOD_TraceFilter(ServerSide);
}

void GMUtility::TraceLine(const Vector& StartPos, const Vector& EndPos, unsigned int Mask, trace_t *ptr)
{
	Ray_t ray;
	ray.Init(StartPos, EndPos);

	// Why have more then one?
	//GMOD_TraceFilter *traceFilter = new GMOD_TraceFilter(ServerSide);

	enginetrace->TraceRay(ray, Mask, traceFilter, ptr);
}
