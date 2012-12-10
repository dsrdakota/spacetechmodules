/*
    gm_navigation
    By Spacetech
*/

#ifndef NODE_H
#define NODE_H

#include "eiface.h"
#include "defines.h"
#include "ILuaModuleManager.h"

class Node
{
public:
	Vector Pos;
	Vector Normal;

	Node(const Vector &Position, const Vector &Norm, Node *Par);
	~Node();
	Node *GetParent();
	void ConnectTo(Node *node, NavDirType Dir);
	Node *GetConnectedNode(NavDirType Dir);
	const Vector *GetPosition();
	const Vector *GetNormal();
	void MarkAsVisited(NavDirType Dir);
	bool HasVisited(NavDirType Dir);
	
	// Stuff for AStar
	void SetStatus(Node* P, float F, float G);
	bool IsOpened();
	void SetOpened(bool Open);
	bool IsDisabled();
	void SetDisabled(bool Disabled);
	bool IsClosed();
	void SetClosed(bool Close);
	float GetScoreF();
	float GetScoreG();
	int GetID();
	void SetID(int id);
	void SetAStarParent(Node* P);
	Node *GetAStarParent();
	void SetNormal(const Vector &Norm);
	void SetPosition(const Vector &Position);

private:
	int ID;
	Node *Parent;

	unsigned short Visited;
	Node *Connections[NUM_DIRECTIONS_MAX];

	Node *AStarParent;
	bool Opened;
	bool Closed;
	bool Disabled;
	float ScoreF;
	float ScoreG;
};

class Nav;
class CJob;
struct JobInfo_t
{
	CJob* job;
	Nav* nav;
	bool abort;
	bool findPath;
	bool foundPath;
	bool hull;
	Vector mins;
	Vector maxs;
	CUtlVector<Node*> path;
	int funcRef;
	time_t updateTime;
	int updateRef;
};

#endif
