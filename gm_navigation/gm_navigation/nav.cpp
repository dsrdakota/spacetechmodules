/*
    gm_navigation
    By Spacetech
*/

#include "nav.h"

Nav::Nav(IThreadPool *threadPool, IFileSystem *filesystem, int GridSize)
{
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

	Nodes.EnsureCapacity(10240);
	groundSeedList.EnsureCapacity(16);
	airSeedList.EnsureCapacity(16);

	this->threadPool = threadPool;

#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "Created Nav\n");
	fs->Close(fh);
#endif
}

Nav::~Nav()
{
#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "Deconstructing Nav\n");
	fs->Close(fh);
#endif
	
	Msg("Deconstructing Nav!?\n");

#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "Freeing Nodes\n");
	fs->Close(fh);
#endif

	Nodes.PurgeAndDeleteElements();
	//Opened.PurgeAndDeleteElements();
   // Closed.PurgeAndDeleteElements();

#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "Freed Nodes\n");
	fs->Close(fh);
#endif
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
		case UP: return DOWN;
		case DOWN: return UP;
		case LEFT:	return RIGHT;
		case RIGHT:	return LEFT;
		case FORWARD: return BACKWARD;
		case BACKWARD: return FORWARD;
	}

	return NORTH;
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
			currentNode = node;
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

		//GMU->TraceLine( from, to, Mask, &result);
		UTIL_TraceLine_GMOD(from, to, Mask, &result);

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

#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "GenerateQueue 1\n");
	fs->Close(fh);
#endif

	ResetGeneration();

#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "GenerateQueue 2\n");
	fs->Close(fh);
#endif

	CJob* j = threadPool->QueueCall(this, &Nav::FullGeneration, &info->abort);

#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "Created Job\n");
	fs->Close(fh);
#endif

	return j;
}

void Nav::FullGeneration(bool *abort)
{
#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "FullGeneration Start\n");
	fs->Close(fh);
#endif

	while(!SampleStep() && *abort == false)
	{
	}

#ifdef FILEBUG
	fh = fs->Open("data/nav/filebug.txt", "a+", "MOD");
	fs->FPrintf(fh, "FullGeneration End\n");
	fs->Close(fh);
#endif
}

void Nav::ResetGeneration()
{
	groundSeedIndex = 0;
	airSeedIndex = 0;
	currentNode = NULL;

	Generating = true;
	Generated = false;
	generatingGround = true;

	Nodes.RemoveAll();
}

void Nav::SetupMaxDistance(const Vector &Pos, int MaxDistance)
{
	Origin = Pos;
	GenerationMaxDistance = MaxDistance;
}

void Nav::AddGroundSeed(const Vector &pos, const Vector &normal)
{
	GroundSeedSpot seed;
	seed.pos = SnapToGrid(pos);
	seed.normal = normal;
	groundSeedList.AddToTail(seed);
}

void Nav::AddAirSeed(const Vector &pos)
{
	AirSeedSpot seed;
	seed.pos = SnapToGrid(pos);
	airSeedList.AddToTail(seed);
}

void Nav::ClearGroundSeeds()
{
	groundSeedList.RemoveAll();
}

void Nav::ClearAirSeeds()
{
	airSeedList.RemoveAll();
}

Node* Nav::GetNextGroundSeedNode()
{
#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextGroundSeedNode: %i\n", groundSeedIndex);
	fs->Flush(fh);
#endif

	if(groundSeedIndex == -1)
	{
#ifdef FILEBUG
		fs->FPrintf(fh, "Invalid Seed Index 1\n");
		fs->Flush(fh);
#endif
		return NULL;
	}

	if(!groundSeedList.IsValidIndex(groundSeedIndex))
	{
#ifdef FILEBUG
		fs->FPrintf(fh, "Invalid Seed Index 2\n");
		fs->Flush(fh);
#endif
		return NULL;
	}

#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextGroundSeedNode: Continuing 1\n");
	fs->Flush(fh);
#endif

	GroundSeedSpot spot = groundSeedList.Element(groundSeedIndex);

#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextGroundSeedNode: Continuing 2\n");
	fs->Flush(fh);
#endif

	groundSeedIndex = groundSeedList.Next(groundSeedIndex);

#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextGroundSeedNode: Continuing 3\n");
	fs->Flush(fh);
#endif

	if(GetNode(spot.pos) == NULL)
	{
		if(GenerationMaxDistance > 0)
		{
			if(Origin.DistTo(spot.pos) > GenerationMaxDistance)
			{
#ifdef FILEBUG
				fs->FPrintf(fh, "GetNextGroundSeedNode: Skipping Seed\n");
				fs->Flush(fh);
#endif
				return GetNextGroundSeedNode();
			}
		}
#ifdef FILEBUG
		fs->FPrintf(fh, "GetNextGroundSeedNode: Adding Node\n");
		fs->Flush(fh);
#endif
		Node *node = new Node(spot.pos, spot.normal, NULL);
		node->SetID(Nodes.AddToTail(node));
		return node;
	}

#ifdef FILEBUG
	fs->FPrintf(fh, "GetNextWalkableSeedNode: Next Seed\n");
	fs->Flush(fh);
#endif

	return GetNextGroundSeedNode();
}

Node *Nav::GetNextAirSeedNode()
{
	if(airSeedIndex == -1)
	{
		return NULL;
	}

	if(!airSeedList.IsValidIndex(airSeedIndex))
	{
		return NULL;
	}

	AirSeedSpot spot = airSeedList.Element(airSeedIndex);

	airSeedIndex = airSeedList.Next(airSeedIndex);

	if(GetNode(spot.pos) == NULL)
	{
		if(GenerationMaxDistance > 0)
		{
			if(Origin.DistTo(spot.pos) > GenerationMaxDistance)
			{
				return GetNextAirSeedNode();
			}
		}
		Node *node = new Node(spot.pos, vector_origin, NULL);
		node->SetID(Nodes.AddToTail(node));
		return node;
	}

	return GetNextAirSeedNode();
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

	// take a ground step
	while(generatingGround)
	{
		if(currentNode == NULL)
		{
			currentNode = GetNextGroundSeedNode();

			if(currentNode == NULL)
			{
				generatingGround = false;
				break;
			}
		}

		//
		// Take a step from this node
		//

#ifdef FILEBUG
		fs->FPrintf(fh, "Stepping From Node\n");
		fs->Flush(fh);
#endif
		for(int dir = NORTH; dir < NumDir; dir++)
		{
#ifdef FILEBUG
			fs->FPrintf(fh, "Checking Direction: %i\n", dir);
			fs->Flush(fh);
#endif
			if(!currentNode->HasVisited((NavDirType)dir))
			{
				// have not searched in this direction yet

#ifdef FILEBUG
				fs->FPrintf(fh, "Unsearched Direction: %i\n", dir);
				fs->Flush(fh);
#endif

				// start at current node position
				Vector Pos = *currentNode->GetPosition();

#ifdef FILEBUG
				fs->FPrintf(fh, "1 <%f, %f>\n", Pos.x, Pos.y);
				fs->Flush(fh);
#endif
				switch(dir)
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
				fs->Flush(fh);
#endif

				generationDir = (NavDirType)dir;

				// mark direction as visited
				currentNode->MarkAsVisited(generationDir);

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
					fs->Flush(fh);
#endif
					return false;
				}

				Vector from = *currentNode->GetPosition();

				Vector fromOrigin = from + addVector;
				Vector toOrigin = to + addVector;

				trace_t result;

				//GMU->TraceLine(fromOrigin, toOrigin, Mask, &result);
				UTIL_TraceLine_GMOD(fromOrigin, toOrigin, Mask, &result);

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
						fs->Flush(fh);
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
								fs->Flush(fh);
#endif
								break;
							}

							// check for maximum allowed slope
							if(normal.z < 0.65)
							{
								walkable = false;
#ifdef FILEBUG
								fs->FPrintf(fh, "Slope\n");
								fs->Flush(fh);
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
					fs->Flush(fh);
#endif
				}

				if(walkable)
				{
					AddNode(to, toNormal, generationDir, currentNode);
				}

				return false;
			}
		}

		// all directions have been searched from this node - pop back to its parent and continue
		currentNode = currentNode->GetParent();
	}

	// Step through air
	while(true)
	{
		if(currentNode == NULL)
		{
			currentNode = GetNextAirSeedNode();
			//Msg("GetNextAirSeedNode\n");
			if(currentNode == NULL)
			{
				break;
			}
		}

		for(int dir = UP; dir < NUM_DIRECTIONS_MAX; dir++) // UP to the end!
		{
			if(!currentNode->HasVisited((NavDirType)dir))
			{
				// start at current node position
				Vector pos = *currentNode->GetPosition();

				switch(dir)
				{
					case UP:		pos.z += GenerationStepSize; break;
					case DOWN:		pos.z -= GenerationStepSize; break;
					case LEFT:		pos.x -= GenerationStepSize; break;
					case RIGHT:		pos.x += GenerationStepSize; break;
					case FORWARD:	pos.y += GenerationStepSize; break;
					case BACKWARD:	pos.y -= GenerationStepSize; break;
				}

				generationDir = (NavDirType)dir;
				currentNode->MarkAsVisited(generationDir);
				
				// Trace from the current node to the pos (Check if we hit anything)
				trace_t result;

				//GMU->TraceLine(*currentNode->GetPosition(), pos, Mask, &result);
				UTIL_TraceLine_GMOD(*currentNode->GetPosition(), pos, Mask, &result);

				if(!result.DidHit())
				{
					//Msg("added node\n");
					AddNode(pos, vector_origin, generationDir, currentNode);
				}

				//Msg("HasVisited: %d %d\n", currentNode->GetID(), dir);

				return false;
			}
		}

		// all directions have been searched from this node - pop back to its parent and continue
		currentNode = currentNode->GetParent();

		//Msg("back to parent\n");
	}

	Generated = true;
	Generating = false;

	return true;
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

	// Save current nav file version
	// buf.PutInt(NAV_VERSION);

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

		for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
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
		for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
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

	FileHandle_t fh = fs->Open(Filename, "a+");
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

	//int fileVersion = buf.GetInt();

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

	while(!info->abort)
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

			for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++) // NumDir
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
					/*
					if(info->hull)
					{
						trace_t result;
						GMU->TraceHull(*current->GetPosition(), *connection->GetPosition(), info->mins, info->maxs, Mask, &result);
						if(result.DidHit())
						{
							continue;
						}
					}
					*/

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