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

	for(int i = NORTH; i < NUM_DIRECTIONS; i++)
	{
		Connections[i] = NULL;
	}

#ifdef DIAGONAL
	for(int i = NORTH; i < NUM_DIRECTIONS; i++)
	{
		Visited[i] = 0;
	}
#else
	Visited = 0;
#endif

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
#ifdef DIAGONAL
	Visited[Dir] = 1;
#else
	Visited |= (1 << Dir);
#endif
}

bool Node::HasVisited(NavDirType Dir)
{
#ifdef DIAGONAL
	return Visited[Dir] == 1;
#else
	if(Visited & (1 << Dir))
	{
		return true;
	}
	return false;
#endif
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
