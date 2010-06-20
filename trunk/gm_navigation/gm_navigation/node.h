/*
    gm_navigation
    By Spacetech
*/

#ifndef NODE_H
#define NODE_H

#include "eiface.h"
#include "gmutility.h"
#include "defines.h"

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
	void SetStatus(Node* P, float F, float G, float H);
	bool IsOpened();
	void SetOpened(bool Open);
	bool IsClosed();
	void SetClosed(bool Close);
	float GetScoreH();
	float GetScoreF();
	float GetScoreG();
	int GetID();
	void SetID(int id);
	void SetAStarParent(Node* P);
	Node *GetAStarParent();

private:
	int ID;
	Node *Parent;

	unsigned char Visited;
	Node *Connections[NUM_DIRECTIONS_MAX];

	Node *AStarParent;
	bool Opened;
	bool Closed;
	float ScoreF;
	float ScoreG;
	float ScoreH;
};

#endif
