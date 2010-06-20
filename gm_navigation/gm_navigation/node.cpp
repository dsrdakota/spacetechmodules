/*
    gm_navigation
    By Spacetech
*/

#include "node.h"

Node::Node(const Vector &Position, const Vector &Norm, Node *Par)
{
	ID = -1;
	Pos = Position;
	Normal = Norm;
	Parent = Par;

	// Not sure if theres any memory problems with this.
	// If it's not diagonal your still setting 4 of them to NULL
	for(int i = NORTH; i < NUM_DIRECTIONS_MAX; i++)
	{
		Connections[i] = NULL;
	}

	Visited = 0;

	Opened = false;
	Closed = false;
	AStarParent = NULL;
}

Node::~Node()
{
	Msg("Deconstructed Node!?\n");
	//delete [] &Connections;
}

void Node::ConnectTo(Node *node, NavDirType Dir)
{
	Connections[Dir] = node;
}

Node *Node::GetConnectedNode(NavDirType Dir)
{
	return Connections[Dir];
}

const Vector *Node::GetPosition()
{
	return &Pos;
}

const Vector *Node::GetNormal()
{
	return &Normal;
}

Node *Node::GetParent()
{
	return Parent;
}

void Node::MarkAsVisited(NavDirType Dir)
{
	Visited |= (1 << Dir);
}

bool Node::HasVisited(NavDirType Dir)
{
	if(Visited & (1 << Dir))
	{
		return true;
	}
	return false;
}

void Node::SetStatus(Node* P, float F, float G, float H)
{
	AStarParent = P;
	ScoreF = F;
	ScoreG = G;
	ScoreH = H;
}

bool Node::IsOpened()
{
	return Opened;
}

void Node::SetOpened(bool Open)
{
	Opened = Open;
}

bool Node::IsClosed()
{
	return Closed;
}

void Node::SetClosed(bool Close)
{
	Closed = Close;
}

Node *Node::GetAStarParent()
{
	return AStarParent;
}

void Node::SetAStarParent(Node* P)
{
	AStarParent = P;
}

float Node::GetScoreH()
{
	return ScoreH;
}

float Node::GetScoreF()
{
	return ScoreF;
}

float Node::GetScoreG()
{
	return ScoreG;
}

int Node::GetID()
{
	return ID;
}

void Node::SetID(int id)
{
	ID = id;
}
