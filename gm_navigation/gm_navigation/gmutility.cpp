/*
    gm_navigation
    By Spacetech
*/

#include "gmutility.h"

GMUtility::GMUtility(IEngineTrace *et, bool serverSide)
{
	enginetrace = et;
	traceFilter = new GMOD_TraceFilter(serverSide);
}

void GMUtility::TraceLine(const Vector& start, const Vector& end, unsigned int mask, trace_t *ptr)
{
	ray.Init(start, end);
	enginetrace->TraceRay(ray, mask, traceFilter, ptr);
}

void GMUtility::TraceHull(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, trace_t *ptr)
{
	rayHull.Init(start, end, mins, maxs);
	enginetrace->TraceRay(rayHull, mask, traceFilter, ptr);
}

