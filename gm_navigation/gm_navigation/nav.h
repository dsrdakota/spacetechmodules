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

#define MAX_GROUND_LAYERS 16

const float HumanHeight = 72.0f;
const float HalfHumanHeight = 36.0f;
const float JumpCrouchHeight = 58.0f; // This shouldn't be constant if your going up stairs with different grid sizes;
const float DeathDrop = 30.0f; // 200.0f; Lets check to make sure we aren't falling down

class Nav
{
public:
	Nav(GMUtility *gmu, IFileSystem *filesystem, int GridSize);
	~Nav();
	void RemoveAllNodes();
	void SetGridSize(int GridSize);
	Node *GetNode(const Vector &Pos);
	Node *GetNodeByID(int ID);
	Node *AddNode(const Vector &destPos, const Vector &normal, NavDirType dir, Node *source);
	int GetGridSize();
	float SnapToGrid(float x);
	Vector SnapToGrid(const Vector& in, bool snapX = true, bool snapY = true);
	float Round(float Val, float Unit);
	NavDirType OppositeDirection(NavDirType Dir);
	bool GetGroundHeight(const Vector &pos, float *height, Vector *normal);
	void StartGeneration();
	void SetupMaxDistance(const Vector &Pos, int MaxDistance);
	void AddWalkableSeed(const Vector &Pos, const Vector &Normal);
	void ClearWalkableSeeds();
	Node *GetNextWalkableSeedNode();
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
	CUtlVector<Node*>& FindPath();
	CUtlVector<Node*>& CalcPath(Node *current);
	void Reset();
	void AddOpenedNode(Node *node);
	void AddClosedNode(Node *node);
	bool HasFoundPath();
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

	enum Heuristic {
		HEURISTIC_MANHATTAN,
		HEURISTIC_EUCLIDEAN,
		HEURISTIC_CUSTOM
	};

private:
	bool Generated;
	bool Generating;

	Vector StartPos;
	Vector AddVector;

	int NumDir;
	bool DiagonalMode;

	int Mask;
	int GenerationStepSize;
	
	GMUtility *GMU;
	IFileSystem *fs;

#ifdef FILEBUG
	FileHandle_t fh;
#endif

	NavDirType GenerationDir;

	Node *CurrentNode;

	Vector Origin;
	int GenerationMaxDistance;

	CUtlVector<Node*> Nodes;
	CUtlVector<Node*> Path;
	CUtlVector<Node*> Opened;
	CUtlVector<Node*> Closed;

	Node *Start;
	Node *End;

	int Heuristic;
	int HeuristicRef;
	bool FoundPath;

	int SeedIndex;
	struct WalkableSeedSpot
	{
		Vector Pos;
		Vector Normal;
	};

	CUtlLinkedList<WalkableSeedSpot, int> WalkableSeedList;
};


#endif
