/*
    gm_navigation
    By Spacetech
*/

#ifndef GMUTILITY
#define GMUTILITY

#include "eiface.h"
#include "engine/ienginetrace.h"
#include "iserverunknown.h"
#include "iclientunknown.h"

class GMOD_TraceFilter : public CTraceFilter
{
public:
	GMOD_TraceFilter(bool ServerSide) :
		m_bServerSide(ServerSide)
	{}

	virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
	{
		if(m_bServerSide)
		{
			IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
			pCollide = pUnk->GetCollideable();
		}
		else
		{
			IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
			pCollide = pUnk->GetCollideable();
		}
		return CheckCollisionGroup(pCollide->GetCollisionGroup());
	}

	bool CheckCollisionGroup(int CollisionGroup)
	{
		if(CollisionGroup == COLLISION_GROUP_PLAYER)
		{
			return false;
		}
		return true;
	}
private:
	bool m_bServerSide;
	ICollideable *pCollide;
};

class GMUtility
{

public:
	GMUtility(IEngineTrace *et, bool ServerSide);
	~GMUtility();

	void TraceLine(const Vector& StartPos, const Vector& EndPos, unsigned int Mask, trace_t *ptr);

private:
	IEngineTrace *enginetrace;
	GMOD_TraceFilter *traceFilter;

};

#endif
