/*
    gm_navigation
    By Spacetech
*/

#ifndef NAV_H
#define NAV_H

#include "node.h"
#include "gmutility.h"
#include "filesystem.h"
#include "defines.h"
#include "utlbuffer.h"
#include "utllinkedlist.h"
#include "jobthread.h"
#include <time.h>

#define MAX_GROUND_LAYERS 16

const float HumanHeight = 72.0f;
const float HalfHumanHeight = 36.0f;
const float JumpCrouchHeight = 58.0f; // This shouldn't be constant if your going up stairs with different grid sizes;
const float DeathDrop = 30.0f; // 200.0f; Lets check to make sure we aren't falling down
const Vector vector_origin = Vector(0, 0, 0);

struct GroundSeedSpot
{
	Vector pos;
	Vector normal;
};

struct AirSeedSpot
{
	Vector pos;
	Vector normal;
};

class Nav
{
public:
	Nav(GMUtility *gmu, IThreadPool *threadPool, IFileSystem *filesystem, int GridSize);
	~Nav();
	static NavDirType OppositeDirection(NavDirType Dir);

	void RemoveAllNodes();
	void SetGridSize(int GridSize);
	Node *GetNode(const Vector &Pos);
	Node *GetNodeByID(int ID);
	Node *AddNode(const Vector &destPos, const Vector &normal, NavDirType dir, Node *source);
	void RemoveNode(Node *node);
	int GetGridSize();
	float SnapToGrid(float x);
	Vector SnapToGrid(const Vector& in, bool snapX = true, bool snapY = true);
	float Round(float Val, float Unit);
	bool GetGroundHeight(const Vector &pos, float *height, Vector *normal);
	CJob* GenerateQueue(JobInfo_t *info);
	void FullGeneration(bool *abort);
	void ResetGeneration();
	void SetupMaxDistance(const Vector &Pos, int MaxDistance);

	void AddGroundSeed(const Vector &pos, const Vector &normal);
	void AddAirSeed(const Vector &pos);
	void ClearGroundSeeds();
	void ClearAirSeeds();
	Node* GetNextGroundSeedNode();
	Node* GetNextAirSeedNode();

	bool SampleStep();
	bool IsGenerated();
	bool Save(const char *Filename);
	bool Load(const char *Filename);
	CUtlVector<Node*>& GetNodes();
	Node *GetClosestNode(const Vector &Pos);

	float HeuristicDistance(const Vector *StartPos, const Vector *EndPos);
	float ManhattanDistance(const Vector *StartPos, const Vector *EndPos);
	float EuclideanDistance(const Vector *StartPos, const Vector *EndPos);

	Node *FindLowestF();
	CJob* FindPathQueue(JobInfo_t *info);
	void ExecuteFindPath(JobInfo_t *info, Node *start, Node *end);
	void Reset();
	void AddOpenedNode(Node *node);
	void AddClosedNode(Node *node);
	bool GetDiagonal();
	void SetDiagonal(bool Diagonal);
	int GetMask();
	void SetMask(int M);
	int GetNumDir();
	int GetHeuristic();
	Node *GetStart();
	Node *GetEnd();
	void SetHeuristic(int H);
	void SetStart(Node *start);
	void SetEnd(Node *end);

	enum Heuristic
	{
		HEURISTIC_MANHATTAN,
		HEURISTIC_EUCLIDEAN,
		HEURISTIC_CUSTOM
	};

private:
	bool Generated;
	bool Generating;
	bool generatingGround;

	Vector StartPos;
	Vector addVector;

	int NumDir;
	bool DiagonalMode;

	int Mask;
	int GenerationStepSize;
	
	GMUtility *GMU;
	IThreadPool *threadPool;
	IFileSystem *fs;
	CThreadMutex lock;
	
#ifdef FILEBUG
	FileHandle_t fh;
#endif

	NavDirType generationDir;
	Node *currentNode;

	Vector Origin;
	int GenerationMaxDistance;

	CUtlVector<Node*> Nodes;
	CUtlVector<Node*> Opened;
	CUtlVector<Node*> Closed;

	Node *Start;
	Node *End;

	int Heuristic;
	int HeuristicRef;

	int groundSeedIndex;
	int airSeedIndex;

	CUtlLinkedList<GroundSeedSpot, int> groundSeedList;
	CUtlLinkedList<AirSeedSpot, int> airSeedList;
};


#endif
