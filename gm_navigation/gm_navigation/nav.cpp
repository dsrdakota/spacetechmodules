/*
    gm_navigation
    By Spacetech
*/

#include "nav.h"

Nav::Nav(GMUtility *gmu, IThreadPool *threadPool, IFileSystem *filesystem, int GridSize)
{
	GMU = gmu;
	fs = filesystem;

	Heuristic = HEURISTIC_MANHATTAN;

	Start = NULL;
	End = NULL;

	Generated = false;
	Generating = false;
	GenerationMaxDistance = -1;

	Mask = MASK_PLAYERSOLID_BRUSHONLY;

	SetDiagonal(false);
	SetGridSize(GridSize);

	// I don't believe this is required but I will leave this here for now.
	// I wanted to make sure that enough memory is reserved but doesn't it dynamically expand
	Nodes.EnsureCapacity(10240);
	WalkableSeedList.EnsureCapacity(16);

	this->threadPool = threadPool;

#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "wb", "MOD");

	fs->FPrintf(fh, "Created Nav\n");
#endif
}

Nav::~Nav()
{
#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "wb", "MOD");

	fs->FPrintf(fh, "Deconstructing Nav\n");
#endif
	
	Msg("Deconstructing Nav!?\n");

#ifdef FILEBUG
		fh = fs->Open("data/nav/filebug.txt", "wb", "MOD");

		fs->FPrintf(fh, "Freeing Nodes\n");
#endif

	Nodes.PurgeAndDeleteElements();
	//Opened.PurgeAndDeleteElements();
   // Closed.PurgeAndDeleteElements();

#ifdef FILEBUG
		fh = fs->Open("data/nav/filebug.txt", "wb", "MOD");

		fs->FPrintf(fh, "Freed Nodes\n");
#endif
}

void Nav::SetGridSize(int GridSize)
{
	GenerationStepSize = GridSize;
	addVector = Vector(0, 0, GridSize / 2);
}

Node *Nav::GetNode(const Vector &Pos)
{
	const float Tolerance = 0.45f * GenerationStepSize;

	Node *Node;
	for(int i = 0; i < Nodes.Count(); i++)
	{
		Node = Nodes[i];
		float dx = fabs(Node->Pos.x - Pos.x);
		float dy = fabs(Node->Pos.y - Pos.y);
		float dz = fabs(Node->Pos.z - Pos.z);
		if(dx < Tolerance && dy < Tolerance && dz < Tolerance)
		{
			return Node;
		}
	}

	return NULL;
}

Node *Nav::GetNodeByID(int ID)
{
	if(!Nodes.IsValidIndex(ID))
	{
		return NULL;
	}
	return Nodes.Element(ID);
}

Node *Nav::AddNode(const Vector &Pos, const Vector &Normal, NavDirType Dir, Node *Source)
{
	if(GenerationMaxDistance > 0)
	{
		if(Origin.DistTo(Pos) > GenerationMaxDistance)
		{
			return NULL;
		}
	}

	// check if a node exists at this location
	Node *node = GetNode(Pos);
	
	// if no node exists, create one
	bool UseNew = false;
	if(node == NULL)
	{
		UseNew = true;
		node = new Node(Pos, Normal, Source);
		node->SetID(Nodes.AddToTail(node));
#ifdef FILEBUG
		fs->FPrintf(fh, "Adding Node <%f, %f>\n", Pos.x, Pos.y);
#endif
	}
	else
	{
#ifdef FILEBUG
		fs->FPrintf(fh, "Using Existing Node <%f, %f>\n", Pos.x, Pos.y);
#endif
	}

	if(Source != NULL)
	{
		// connect source node to new node
		Source->ConnectTo(node, Dir);

		node->ConnectTo(Source, OppositeDirection(Dir));
		node->MarkAsVisited(OppositeDirection(Dir));

		if(UseNew)
		{
			// new node becomes current node
			CurrentNode = node;
		}
	}

	/* hmmmmm
	// optimization: if deltaZ changes very little, assume connection is commutative
	const float zTolerance = 50.0f;
	if(fabs(Source->GetPosition()->z - Pos.z) < zTolerance)
	{
		Node->ConnectTo(Source, OppositeDirection(Dir));
		Node->MarkAsVisited(OppositeDirection(Dir));
	}
	*/

	return node;
}

void Nav::RemoveNode(Node *node)
{
	Node *Connection;
	for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
	{
		Connection = node->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL)
		{
			Connection->ConnectTo(NULL, OppositeDirection((NavDirType)Dir));
		}
	}

	Nodes.Remove(node->GetID());

	// Update all node ids, removing a element from a utlvector will shift all the elements positions
	for(int i = 0; i < Nodes.Count(); i++)
	{
		Nodes[i]->SetID(i);
	}
}

int Nav::GetGridSize()
{
	return GenerationStepSize;
}

bool Nav::IsGenerated()
{
	return Generated;
}

float Nav::SnapToGrid(float x)
{
	return Round(x, GetGridSize());
}

Vector Nav::SnapToGrid(const Vector& in, bool snapX, bool snapY)
{
	int scale = GetGridSize();

	Vector out(in);

	if(snapX)
	{
		out.x = Round(in.x, scale);
	}

	if(snapY)
	{
		out.y = Round(in.y, scale);
	}

	return out;
}

float Nav::Round(float Val, float Unit)
{
	Val = Val + ((Val < 0.0f) ? -Unit*0.5f : Unit*0.5f);
	return (float)(Unit * ((int)Val) / (int)Unit);
}

NavDirType Nav::OppositeDirection(NavDirType Dir)
{
	switch(Dir)
	{
		case NORTH: return SOUTH;
		case SOUTH: return NORTH;
		case EAST:	return WEST;
		case WEST:	return EAST;
		case NORTHEAST: return SOUTHWEST;
		case NORTHWEST: return SOUTHEAST;
		case SOUTHEAST:	return NORTHWEST;
		case SOUTHWEST: return NORTHEAST;
	}

	return NORTH;
}

bool Nav::GetGroundHeight(const Vector &pos, float *height, Vector *normal)
{
	Vector to;
	to.x = pos.x;
	to.y = pos.y;
	to.z = pos.z - 9999.9f;

	float offset;
	Vector from;
	trace_t result;

	const float maxOffset = 100.0f;
	const float inc = 10.0f;

	struct GroundLayerInfo
	{
		float ground;
		Vector normal;
	}
	layer[ MAX_GROUND_LAYERS ];
	int layerCount = 0;

	for( offset = 1.0f; offset < maxOffset; offset += inc )
	{
		from = pos + Vector( 0, 0, offset );

		GMU->TraceLine( from, to, Mask, &result);

		// if the trace came down thru a door, ignore the door and try again
		// also ignore breakable floors

		if (result.startsolid == false)
		{
			// if we didnt start inside a solid area, the trace hit a ground layer

			// if this is a new ground layer, add it to the set
			if (layerCount == 0 || result.endpos.z > layer[ layerCount-1 ].ground)
			{
				layer[ layerCount ].ground = result.endpos.z;
				if (result.plane.normal.IsZero())
					layer[ layerCount ].normal = Vector( 0, 0, 1 );
				else
					layer[ layerCount ].normal = result.plane.normal;

				++layerCount;
						
				if (layerCount == MAX_GROUND_LAYERS)
					break;
			}
		}
	}

	if (layerCount == 0)
		return false;

	// find the lowest layer that allows a player to stand or crouch upon it
	int i;
	for( i=0; i<layerCount-1; ++i )
	{
		if (layer[i+1].ground - layer[i].ground >= HumanHeight)
			break;		
	}

	*height = layer[ i ].ground;

	if (normal)
		*normal = layer[ i ].normal;

	return true;
}

CJob* Nav::GenerateQueue(JobInfo_t *info)
{
	if(Generating)
	{
		return NULL;
	}

	ResetGeneration();

	return threadPool->QueueCall(this, &Nav::FullGeneration, &info->abort);
}

void Nav::FullGeneration(bool *abort)
{
#ifdef FILEBUG
		fh = fs->Open("data/nav/filebug.txt", "wb", "MOD");

		fs->FPrintf(fh, "FullGeneration Start\n");
#endif

	while(!SampleStep() && *abort == false)
	{
	}

#ifdef FILEBUG
		fh = fs->Open("data/nav/filebug.txt", "wb", "MOD");

		fs->FPrintf(fh, "FullGeneration End\n");
#endif
}

void Nav::ResetGeneration()
{
	SeedIndex = 0;
	CurrentNode = NULL;

	Generating = true;
	Generated = false;

	Nodes.RemoveAll();
}

void Nav::SetupMaxDistance(const Vector &Pos, int MaxDistance)
{
	Origin = Pos;
	GenerationMaxDistance = MaxDistance;
}

void Nav::AddWalkableSeed(const Vector &Pos, const Vector &Normal)
{
	WalkableSeedSpot Seed;

	Seed.Pos = SnapToGrid(Pos);
	Seed.Normal = Normal;

	WalkableSeedList.AddToTail(Seed);
}

void Nav::ClearWalkableSeeds()
{
	WalkableSeedList.RemoveAll();
}

Node *Nav::GetNextWalkableSeedNode()
{
#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextWalkableSeedNode: %i\n", SeedIndex);
#endif

	if(SeedIndex == -1)
	{
#ifdef FILEBUG
		fs->FPrintf(fh, "Invalid Seed Index 1\n");
#endif
		return NULL;
	}

	if(!WalkableSeedList.IsValidIndex(SeedIndex))
	{
#ifdef FILEBUG
		fs->FPrintf(fh, "Invalid Seed Index 2\n");
#endif
		return NULL;
	}

#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextWalkableSeedNode: Continuing 1\n");
#endif

	WalkableSeedSpot Spot = WalkableSeedList.Element(SeedIndex);

#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextWalkableSeedNode: Continuing 2\n");
#endif

	SeedIndex = WalkableSeedList.Next(SeedIndex);

#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextWalkableSeedNode: Continuing 3\n");
#endif

	if(GetNode(Spot.Pos) == NULL)
	{
		if(GenerationMaxDistance > 0)
		{
			if(Origin.DistTo(Spot.Pos) > GenerationMaxDistance)
			{
#ifdef FILEBUG
				fs->FPrintf(fh, "GetNextWalkableSeedNode: Skipping Seed\n");
#endif
				return GetNextWalkableSeedNode();
			}
		}
#ifdef FILEBUG
		fs->FPrintf(fh, "GetNextWalkableSeedNode: Adding Node\n");
#endif
		Node *node = new Node(Spot.Pos, Spot.Normal, NULL);
		node->SetID(Nodes.AddToTail(node));
		return node;
	}

#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextWalkableSeedNode: Next Seed\n");
#endif

	return GetNextWalkableSeedNode();
}

bool Nav::SampleStep()
{
	if(IsGenerated())
	{
		return true;
	}

	if(!Generating)
	{
		return true;
	}

	// take a step
	while(true)
	{
		if(CurrentNode == NULL)
		{
			CurrentNode = GetNextWalkableSeedNode();

			if(CurrentNode == NULL)
			{
				Generated = true;
				Generating = false;
				return true;
			}
		}

		//
		// Take a step from this node
		//

#ifdef FILEBUG
		fs->FPrintf(fh, "Stepping From Node\n");
#endif
		for(int Dir = NORTH; Dir < NumDir; Dir++)
		{
#ifdef FILEBUG
			fs->FPrintf(fh, "Checking Direction: %i\n", Dir);
#endif
			if(!CurrentNode->HasVisited((NavDirType)Dir))
			{
				// have not searched in this direction yet

#ifdef FILEBUG
				fs->FPrintf(fh, "Unsearched Direction: %i\n", Dir);
#endif

				// start at current node position
				Vector Pos = *CurrentNode->GetPosition();

#ifdef FILEBUG
				fs->FPrintf(fh, "1 <%f, %f>\n", Pos.x, Pos.y);
#endif
				switch(Dir)
				{
					case NORTH:	Pos.y += GenerationStepSize; break;
					case SOUTH:	Pos.y -= GenerationStepSize; break;
					case EAST:	Pos.x += GenerationStepSize; break;
					case WEST:	Pos.x -= GenerationStepSize; break;
					case NORTHEAST:	Pos.x += GenerationStepSize; Pos.y += GenerationStepSize; break;
					case NORTHWEST:	Pos.x -= GenerationStepSize; Pos.y += GenerationStepSize; break;
					case SOUTHEAST: Pos.x += GenerationStepSize; Pos.y -= GenerationStepSize; break;
					case SOUTHWEST:	Pos.x -= GenerationStepSize; Pos.y -= GenerationStepSize; break;
				}

#ifdef FILEBUG
				fs->FPrintf(fh, "2 <%f, %f>\n\n", Pos.x, Pos.y);
#endif

				GenerationDir = (NavDirType)Dir;

				// mark direction as visited
				CurrentNode->MarkAsVisited(GenerationDir);

				// test if we can move to new position
				Vector to;

				// modify position to account for change in ground level during step
				to.x = Pos.x;
				to.y = Pos.y;

				Vector toNormal;

				if(GetGroundHeight(Pos, &to.z, &toNormal) == false)
				{
#ifdef FILEBUG
					fs->FPrintf(fh, "Ground Height Fail\n");
#endif
					return false;
				}

				Vector from = *CurrentNode->GetPosition();

				Vector fromOrigin = from + addVector;
				Vector toOrigin = to + addVector;

				trace_t result;

				GMU->TraceLine(fromOrigin, toOrigin, Mask, &result);

				bool walkable;

				if(result.fraction == 1.0f && !result.startsolid)
				{
					// the trace didnt hit anything - clear

					float toGround = to.z;
					float fromGround = from.z;

					float epsilon = 0.1f;

					// check if ledge is too high to reach or will cause us to fall to our death
					// Using GenerationStepSize instead of JumpCrouchHeight so that stairs will work with different grid sizes
					if(toGround - fromGround > GenerationStepSize + epsilon || fromGround - toGround > DeathDrop)
					{
						walkable = false;
#ifdef FILEBUG
						fs->FPrintf(fh, "Bad Ledge\n");
#endif
					}
					else
					{
						// check surface normals along this step to see if we would cross any impassable slopes
						Vector delta = to - from;
						const float inc = 2.0f;
						float along = inc;
						bool done = false;
						float ground;
						Vector normal;

						walkable = true;

						while(!done)
						{
							Vector p;

							// need to guarantee that we test the exact edges
							if(along >= GenerationStepSize)
							{
								p = to;
								done = true;
							}
							else
							{
								p = from + delta * (along / GenerationStepSize);
							}

							if(GetGroundHeight(p, &ground, &normal) == false)
							{
								walkable = false;
#ifdef FILEBUG
								fs->FPrintf(fh, "Bad Node Path\n");
#endif
								break;
							}

							// check for maximum allowed slope
							if(normal.z < 0.65)
							{
								walkable = false;
#ifdef FILEBUG
								fs->FPrintf(fh, "Slope\n");
#endif
								break;
							}

							along += inc;					
						}
					}
				}
				else // TraceLine hit something...
				{
					walkable = false;
#ifdef FILEBUG
					fs->FPrintf(fh, "Hit Something\n");
#endif
				}

				if(walkable)
				{
					AddNode(to, toNormal, GenerationDir, CurrentNode);
				}

				return false;
			}
		}

		// all directions have been searched from this node - pop back to its parent and continue
		CurrentNode = CurrentNode->GetParent();
	}
}

bool Nav::Save(const char *Filename)
{
	if(Generating)
	{
		return false;
	}

	CUtlBuffer buf;

	Node *node, *connection;

	int TotalConnections = 0;
	int NodeTotal = Nodes.Count();

	//////////////////////////////////////////////
	// Nodes
	buf.PutInt(NodeTotal);

	for(int i = 0; i < NodeTotal; i++)
	{
		node = Nodes[i];

		buf.PutFloat(node->Pos.x);
		buf.PutFloat(node->Pos.y);
		buf.PutFloat(node->Pos.z);

		buf.PutFloat(node->Normal.x);
		buf.PutFloat(node->Normal.y);
		buf.PutFloat(node->Normal.z);

		for(int Dir = NORTH; Dir < NumDir; Dir++)
		{
			if(node->GetConnectedNode((NavDirType)Dir) != NULL)
			{
				TotalConnections++;
			}
		}
	}
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	// Connections
	buf.PutInt(TotalConnections);

	for(int i = 0; i < NodeTotal; i++)
	{
		node = Nodes[i];
		for(int Dir = NORTH; Dir < NumDir; Dir++)
		{
			connection = node->GetConnectedNode((NavDirType)Dir);
			if(connection != NULL)
			{
				buf.PutInt(Dir);
				buf.PutInt(node->GetID());
				buf.PutInt(connection->GetID());
			}
		}
	}
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	// Write File

	FileHandle_t fh = fs->Open(Filename, "wb");
	if(!fh)
	{
		return false;
	}

	fs->Write(buf.Base(), buf.TellPut(), fh);
	fs->Close(fh);
	//////////////////////////////////////////////

	return true;
}

bool Nav::Load(const char *Filename)
{
	if(Generating)
	{
		return false;
	}

	CUtlBuffer buf;

	if(!fs->ReadFile(Filename, "MOD", buf))
	{
		return false;
	}

	buf.SeekGet(CUtlBuffer::SEEK_HEAD, 0);

	Node *node;
	int Dir, SrcID, DestID;

	Nodes.RemoveAll();

	//////////////////////////////////////////////
	// Nodes
	int NodeTotal = buf.GetInt();

	for(int i = 0; i < NodeTotal; i++)
	{
		Vector Pos, Normal;
		Pos.x = buf.GetFloat();
		Pos.y = buf.GetFloat();
		Pos.z = buf.GetFloat();

		Normal.x = buf.GetFloat();
		Normal.y = buf.GetFloat();
		Normal.z = buf.GetFloat();

		node = new Node(Pos, Normal, NULL);
		node->SetID(Nodes.AddToTail(node));
	}
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	// Connections
	int TotalConnections = buf.GetInt();

	for(int i = 0; i < TotalConnections; i++)
	{
		Dir = buf.GetInt();
		SrcID = buf.GetInt();
		DestID = buf.GetInt();
		Nodes[SrcID]->ConnectTo(Nodes[DestID], (NavDirType)Dir);
	}

	//////////////////////////////////////////////

	return true;
}

CUtlVector<Node*>& Nav::GetNodes()
{
	return Nodes;
}

Node *Nav::GetClosestNode(const Vector &Pos)
{
	float NodeDist;
	float Distance = -1;
	
	Node *Node, *ClosestNode;

	for(int i = 0; i < Nodes.Count(); i++)
	{
		Node = Nodes[i];
		NodeDist = Pos.DistTo(Node->Pos);
		if(Distance == -1 || NodeDist < Distance)
		{
			ClosestNode = Node;
			Distance = NodeDist;
		}
	}

	return ClosestNode;
}

void Nav::Reset()
{
	Node *node;
	for(int i = 0; i < Nodes.Count(); i++)
	{
		node = Nodes[i];
		node->SetClosed(false);
		node->SetOpened(false);
	}
	Opened.RemoveAll();
	Closed.RemoveAll();
}

void Nav::AddOpenedNode(Node *node)
{
	node->SetOpened(true);
	Opened.AddToTail(node);
}

void Nav::AddClosedNode(Node *node)
{
	node->SetClosed(true);
	bool Removed = Opened.FindAndRemove(node);
	if(!Removed)
	{
		Msg("Failed to remove Node!?\n");
	}
}

float Nav::HeuristicDistance(const Vector *StartPos, const Vector *EndPos)
{
	if(Heuristic == HEURISTIC_MANHATTAN)
	{
		return ManhattanDistance(StartPos, EndPos);
	}
	else if(Heuristic == HEURISTIC_EUCLIDEAN)
	{
		return EuclideanDistance(StartPos, EndPos);
	}
	else if(Heuristic == HEURISTIC_CUSTOM)
	{
		//gLua->PushReference(HeuristicRef);
			//GMOD_PushVector(StartPos);
			//GMOD_PushVector(EndPos);
		//gLua->Call(2, 1);
		//return gLua->GetNumber(1);
	}
	return NULL;
}

float Nav::ManhattanDistance(const Vector *StartPos, const Vector *EndPos)
{
	return (abs(EndPos->x - StartPos->x) + abs(EndPos->y - StartPos->y) + abs(EndPos->z - StartPos->z));
}

// u clid e an
float Nav::EuclideanDistance(const Vector *StartPos, const Vector *EndPos)
{
	return sqrt(pow(EndPos->x - StartPos->x, 2) + pow(EndPos->y - StartPos->y, 2) + pow(EndPos->z - StartPos->z, 2));
}

Node *Nav::FindLowestF()
{
	float BestScoreF = NULL;
	Node *node, *winner = NULL;
	for(int i = 0; i < Opened.Count(); i++)
	{
		node = Opened[i];
		if(BestScoreF == NULL || node->GetScoreF() < BestScoreF)
		{
			winner = node;
			BestScoreF = node->GetScoreF();
		}
	}
	return winner;
}

CJob* Nav::FindPathQueue(JobInfo_t *info)
{
	return threadPool->QueueCall(this, &Nav::ExecuteFindPath, info, Start, End);
}

void Nav::ExecuteFindPath(JobInfo_t *info, Node *start, Node *end)
{
	lock.Lock();
	Reset();

	info->foundPath = false;

	if(start == NULL || end == NULL || start == end)
	{
		lock.Unlock();
		return;
	}

	AddOpenedNode(start);
	start->SetStatus(NULL, HeuristicDistance(start->GetPosition(), end->GetPosition()), 0);

	float currentScoreG, scoreG;
	Node *current = NULL, *connection = NULL;
	
#ifdef FILEBUG
	fs->FPrintf(fh, "Added Node\n");
#endif

	while(true && !info->abort)
	{
		current = FindLowestF();

		if(current == NULL)
		{
			break;
		}

		if(current == end)
		{
			info->foundPath = true;
			break;
		}
		else
		{
			AddClosedNode(current);

			currentScoreG = current->GetScoreG();

			for(int Dir = NORTH; Dir < NumDir; Dir++)
			{
				connection = current->GetConnectedNode((NavDirType)Dir);

				if(connection == NULL)
				{
					continue;
				}

				if(connection->IsClosed() || connection->IsDisabled())
				{
					continue;
				}
				
				scoreG = currentScoreG + EuclideanDistance(current->GetPosition(), connection->GetPosition()); // dist_between here

				if(!connection->IsOpened() || scoreG < connection->GetScoreG())
				{
					if(info->hull)
					{
						trace_t result;
						GMU->TraceHull(*current->GetPosition(), *connection->GetPosition(), info->mins, info->maxs, Mask, &result);
						if(result.DidHit())
						{
							continue;
						}
					}

					AddOpenedNode(connection);
					connection->SetStatus(current, scoreG + HeuristicDistance(connection->GetPosition(), end->GetPosition()), scoreG);
				}
			}
		}
	}

	while(true)
	{
		if(current == NULL)
		{
			break;
		}
		info->path.AddToHead(current);
		current = current->GetAStarParent();
	}

	lock.Unlock();
}

int Nav::GetMask()
{
	return Mask;
}

void Nav::SetMask(int M)
{
	Mask = M;
}

bool Nav::GetDiagonal()
{
	return DiagonalMode;
}

void Nav::SetDiagonal(bool Diagonal)
{
	DiagonalMode = Diagonal;
	if(DiagonalMode)
	{
		NumDir = NUM_DIRECTIONS_DIAGONAL;
	}
	else
	{
		NumDir = NUM_DIRECTIONS;
	}
}

int Nav::GetNumDir()
{
	return NumDir;
}

int Nav::GetHeuristic()
{
	return Heuristic;
}

Node *Nav::GetStart()
{
	return Start;
}

Node *Nav::GetEnd()
{
	return End;
}

void Nav::SetHeuristic(int H)
{
	Heuristic = H;
}

void Nav::SetStart(Node *start)
{
	Start = start;
}

void Nav::SetEnd(Node *end)
{
	End = end;
}