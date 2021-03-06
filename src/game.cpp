#include "game.h"

namespace Blocks
{

using namespace DirectX;

// player vars
// Player spawns looking down X+, with Z+ to his left
// Left-handed coordinate system
XMFLOAT3 playerPos 		= { 133.5f, 60.0f, 3.5f };
XMFLOAT3 playerDir 		= { 1.0f,  0.0f, 0.0f };
XMFLOAT3 playerLook 	= { 1.0f,  0.0f, 0.0f };
XMFLOAT3 playerRight 	= { 0.0f,  0.0f, -1.0f };
XMFLOAT3 playerUp 		= { 0.0f,  1.0f, 0.0f };
XMFLOAT3 playerSpeed 	= { 0.0f,  0.0f, 0.0f };
XMFLOAT3 gravity 		= { 0.0f, -9.8f, 0.0f };

XMMATRIX playerView;
XMMATRIX playerProj;

float playerMass = 75.0f;
float playerHeight = 1.8f;
float playerReach = 4.0f;
bool  playerAirborne = true;

float gPlayerHFOV = 100.0f;
float gPlayerNearPlane = 0.1f;
float gPlayerFarPlane = 1.0f;


bool Game::isInstantiated_ = false;

// scene manager?
GameObject gGameObjects[ MAX_GAME_OBJECTS ];
GameObject * gRootNode;
GameObject * gPlayerNode;
uint gGameObjectCount = 0;
Mesh gFireMesh;
Material gFireMaterial;

GameObject::GameObject()
{
	name = "Unknown";
}

void GameObject::SetPosition(DirectX::XMFLOAT3 position)
{
	transform.position = position;
	this->position = XMMatrixTranslation(transform.position.x,
								   transform.position.y,
								   transform.position.z);

}
void GameObject::SetRotation(DirectX::XMFLOAT3 rotation)
{
	transform.rotation = rotation;
	this->rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(transform.rotation.x),
												  XMConvertToRadians(transform.rotation.y),
												  XMConvertToRadians(transform.rotation.z));
}
void GameObject::SetScale(float scale)
{
	transform.scale = scale;
	this->scale = XMMatrixScaling(scale, scale, scale);
}

GameObject * GameObject::GetChild(const char * name)
{
	GameObject * child;
	for(int i = 0; i < childrenCount; ++i)
	{
		if(strcmp(children[i]->name, name) == 0)
		{
			return children[i];
		}
		if(child = children[i]->GetChild(name))
		{
			return child;
		}
	}
	return 0;
}

void AttachNodeChild(GameObject * parent, GameObject * child)
{
	parent->children[parent->childrenCount] = child;
	parent->childrenCount++;
}

void UpdateNode(GameObject * node, const XMMATRIX & parentWorld)
{
	node->world = node->rotation * node->scale * node->position * parentWorld;
	node->rotationNormalMap = node->world;

	for(int i = 0; i < node->childrenCount; ++i)
	{
		UpdateNode(node->children[i], node->world);
	}
}

// caches
int gChunkCacheHalfDim = 0;
int gChunkCacheDim = 0;
int gChunkMetaCacheHalfDim = 0;
int gChunkMetaCacheDim = 0;
int gChunkMeshCacheHalfDim = 0;
int gChunkMeshCacheDim = 0;
Chunk * gChunkCache = 0;
Chunk * gChunkMetaCache = 0;
ChunkMesh * gChunkMeshCache = 0;
VertexBuffer * gChunkVertexBuffer = 0;

const int gNumShadowCascades = 4;

ResourceHandle gSMRasterizerState[ gNumShadowCascades ];
DepthBuffer gShadowDB(4);

// XMFLOAT3 gSunDirection;
XMFLOAT4 gAmbientColor;
XMFLOAT4 gSunColor;
float gSunElevation = 90.0f;

#define SM_RESOLUTION 2048

void GameTime::AdvanceTime( float ms )
{
	float deltaSec = ms / 1000.0f;
	seconds += deltaSec * scale;
	float minutesPassed = seconds / 60.0f;
	if( minutesPassed >= 1.0f )
	{
		seconds -= (int)minutesPassed * 60.0f;
		minutes++;
		if( minutes == 60 )
		{
			minutes = 0;
			hours++;
			if( hours == 24 )
			{
				hours = 0;
				day++;
				if( day == 31 )
				{
					day = 0;
					month++;
					if( month == 13 )
					{
						month = 1;
						year++;
					}
				}
			}
		}
	}
	elapsedMS += ms;
}

void GameTime::DecreaseTime( float ms )
{
	float deltaSec = ms / 1000.0f;
	seconds -= deltaSec * scale;
	float minutesPassed = -seconds / 60.0f;
	if( minutesPassed >= 1.0f )
	{
		seconds += (int)minutesPassed * 60.0f;
		minutes--;
		if( minutes == -1 )
		{
			minutes = 60;
			hours--;
			if( hours == -1 )
			{
				hours = 23;
				day--;
				if( day == 0 )
				{
					day = 30;
					month--;
					if( month == 0 )
					{
						month = 12;
						year--;
					}
				}
			}
		}
	}
	elapsedMS -= ms;
}

Game::Game()
:
renderer()
{
	assert( !isInstantiated_ );
	isInstantiated_ = true;
}

Game::~Game()
{
	//for( int i = 0; i < gChunkMeshCacheDim * gChunkMeshCacheDim; ++i )
	//{
	//	gChunkVertexBuffer.Release();
	//}
	if( gChunkVertexBuffer )
		delete gChunkVertexBuffer;
	if( gChunkCache )
		delete[] gChunkCache;
	if( gChunkMetaCache )
		delete[] gChunkMetaCache;
	if( gChunkMeshCache )
		delete[] gChunkMeshCache;
}

bool Game::Start( HWND wnd )
{
	if( !renderer.Start( wnd ) )
	{
		return false;
	}

	//int biases[ gNumShadowCascades ] = { 100, 200, 400, 800 };
	//float slopeBiases[ gNumShadowCascades ] = { 1.0f, 1.5f, 1.8f, 3.0f };
	int biases[ gNumShadowCascades ] = { 0, 0, 0, 0 };
	float slopeBiases[ gNumShadowCascades ] = { 0.35f, 0.5f, 0.5f, 0.5f };
	// float slopeBiases[ gNumShadowCascades ] = { 0, 0, 0, 0 };
	RasterizerStateDesc rasterizerStateDesc;
	ZERO_MEMORY( rasterizerStateDesc );
	rasterizerStateDesc.fillMode = FILL_MODE::SOLID;
	rasterizerStateDesc.cullMode = CULL_MODE::NONE;
	rasterizerStateDesc.frontCCW = true;
	rasterizerStateDesc.depthClipEnabled = true;
	for( int i = 0; i < gNumShadowCascades; ++i )
	{
		rasterizerStateDesc.depthBias = biases[i];
		rasterizerStateDesc.depthBiasClamp = 0;
		rasterizerStateDesc.slopeScaledDepthBias = slopeBiases[i];

		gSMRasterizerState[ i ] = renderer.CreateRasterizerState( rasterizerStateDesc );
		if( gSMRasterizerState[ i ] == INVALID_HANDLE )
		{
			return false;
		}
	}

	if( !overlay.Start( &renderer ) )
	{
		return false;
	}
	for( int i = 0; i < KEY::COUNT; i++ )
	{
		input.key[i] = { 0 };
	}
	input.mouse = {0, 0};

	// gameTime_ = { 2015, 1, 1, 12, 0, 0.0f, 60.0f*60.0f };
	gameTime_ = { 2015, 1, 1, 12, 0, 0.0f, 0.0f, 60.0f };

	gPlayerFarPlane = float(Config.viewDistanceChunks * CHUNK_WIDTH);

	gChunkCacheHalfDim = Config.viewDistanceChunks + 1;
	gChunkCacheDim = gChunkCacheHalfDim * 2 + 1;

	gChunkMetaCacheHalfDim = gChunkCacheHalfDim + 1;
	gChunkMetaCacheDim = gChunkCacheDim + 2;

	gChunkMeshCacheHalfDim = Config.viewDistanceChunks;
	gChunkMeshCacheDim = gChunkMeshCacheHalfDim * 2 + 1;

	gChunkCache = new Chunk[ gChunkCacheDim * gChunkCacheDim ];
	gChunkMetaCache = new Chunk[ gChunkMetaCacheDim * gChunkMetaCacheDim ];
	gChunkMeshCache = new ChunkMesh[ gChunkMeshCacheDim * gChunkMeshCacheDim ];
	gChunkVertexBuffer = new VertexBuffer( gChunkMeshCacheDim * gChunkMeshCacheDim );

	InitWorldGen();
	InitChunkCache( gChunkCache, gChunkCacheDim );
	InitChunkCache( gChunkMetaCache, gChunkMetaCacheDim );

	for( int i = 0; i < gChunkMeshCacheDim * gChunkMeshCacheDim; i++ ) {
		gChunkMeshCache[i].dirty = true;
		gChunkMeshCache[i].vertices = 0;
		gChunkMeshCache[i].numVertices = 0;
		gChunkMeshCache[i].chunkPos[0] = 0;
		gChunkMeshCache[i].chunkPos[1] = 0;
	}

#if 0
	for( int i = 0; i < gNumShadowCascades; ++i )
	{
//		if( !gShadowRT[i].Init( SM_RESOLUTION, SM_RESOLUTION, DXGI_FORMAT_R32_FLOAT, true, renderer.GetDevice() ) )
//		{
//			return false;
//		}
		if( !gShadowDB[i].Init( SM_RESOLUTION, SM_RESOLUTION, DXGI_FORMAT_D32_FLOAT, 1, 0, true, renderer.GetDevice() ) )
		{
			return false;
		}
	}
#else
	if( !gShadowDB.Init( SM_RESOLUTION, SM_RESOLUTION, DXGI_FORMAT_D32_FLOAT, 1, 0, true, renderer.GetDevice() ) )
	{
		return false;
	}
#endif


//	gSunDirection = Normalize( { 0.5f, 0.8f, 0.25f } );
	gAmbientColor = { 0.5f, 0.6f, 0.75f, 1.0f };
	gSunColor = 	{ 1.0f, 0.9f, 0.8f, 1.0f };

	playerProj = XMMatrixPerspectiveFovLH( XMConvertToRadians( 60.0f ), (float)Config.screenWidth / (float)Config.screenHeight, 0.1f, 1000.0f );

	// root
	gGameObjects[gGameObjectCount] = GameObject();
	gGameObjects[gGameObjectCount].renderable = false;
	gGameObjects[gGameObjectCount].meshID = (MESH)0;
	gGameObjects[gGameObjectCount].SetPosition({ 0.0f, 0.0f, 0.0f });
	gGameObjects[gGameObjectCount].SetRotation({ 0.0f, 0.0f, 0.0f });
	gGameObjects[gGameObjectCount].SetScale(1.0f);
	gGameObjects[gGameObjectCount].childrenCount = 0;
	gGameObjects[gGameObjectCount].name = "WorldRoot";
	gRootNode = &gGameObjects[gGameObjectCount];
	gGameObjectCount++;

	// campfire
	gGameObjects[gGameObjectCount] = GameObject();
	gGameObjects[gGameObjectCount].renderable = true;
	gGameObjects[gGameObjectCount].meshID = MESH_CAMPFIRE;
	gGameObjects[gGameObjectCount].SetPosition({ 134.5f, 33.0f, 21.5f });
	gGameObjects[gGameObjectCount].SetRotation({ 0.0f, 180.0f, 0.0f });
	gGameObjects[gGameObjectCount].SetScale(1.0f);
	gGameObjects[gGameObjectCount].childrenCount = 0;
	gGameObjects[gGameObjectCount].name = "CampFire";

	AttachNodeChild(gRootNode, &gGameObjects[gGameObjectCount]);

	gGameObjectCount++;

	// player
	gGameObjects[gGameObjectCount] = GameObject();
	gGameObjects[gGameObjectCount].renderable = false;
	gGameObjects[gGameObjectCount].meshID = (MESH)0;
	gGameObjects[gGameObjectCount].SetPosition({ 0.0f, 0.0f, 0.0f });
	gGameObjects[gGameObjectCount].SetRotation({ 0.0f, 0.0f, 0.0f });
	gGameObjects[gGameObjectCount].SetScale(1.0f);
	gGameObjects[gGameObjectCount].childrenCount = 0;
	gGameObjects[gGameObjectCount].name = "PlayerRoot";

	gPlayerNode = &gGameObjects[gGameObjectCount];

	AttachNodeChild(gRootNode, &gGameObjects[gGameObjectCount]);

	gGameObjectCount++;


	// TODO: held weapon is only visible if the soulder is eye-high due
	//  to the narrow FOV. Do I want to do anything about it?
	// bone_shoulder_r
	gGameObjects[gGameObjectCount] = GameObject();
	gGameObjects[gGameObjectCount].renderable = false;
	gGameObjects[gGameObjectCount].meshID = (MESH)0;
	gGameObjects[gGameObjectCount].SetPosition({ 0.0f, playerHeight, -0.17f });
	gGameObjects[gGameObjectCount].SetRotation({ 0.0f, 0.0f, 0.0f });
	gGameObjects[gGameObjectCount].SetScale(1.0f);
	gGameObjects[gGameObjectCount].childrenCount = 0;
	gGameObjects[gGameObjectCount].name = "BoneShoulderR";

	AttachNodeChild(gPlayerNode, &gGameObjects[gGameObjectCount]);

	gGameObjectCount++;

	// bone_forearm_r
	gGameObjects[gGameObjectCount] = GameObject();
	gGameObjects[gGameObjectCount].renderable = false;
	gGameObjects[gGameObjectCount].meshID = (MESH)0;
	gGameObjects[gGameObjectCount].SetPosition({ 0.0f, -0.3f, 0.0f });
	gGameObjects[gGameObjectCount].SetRotation({ 0.0f, 0.0f, 90.0f });
	gGameObjects[gGameObjectCount].SetScale(1.0f);
	gGameObjects[gGameObjectCount].childrenCount = 0;
	gGameObjects[gGameObjectCount].name = "BoneForearmR";

	AttachNodeChild(gPlayerNode->GetChild("BoneShoulderR"), &gGameObjects[gGameObjectCount]);

	gGameObjectCount++;

	// bone_hand_r / weapon slot
	gGameObjects[gGameObjectCount] = GameObject();
	gGameObjects[gGameObjectCount].renderable = false;
	gGameObjects[gGameObjectCount].meshID = (MESH)0;
	gGameObjects[gGameObjectCount].SetPosition({ 0.0f, -0.35f, 0.0f });
	gGameObjects[gGameObjectCount].SetRotation({ 0.0f, 0.0f, 0.0f });
	gGameObjects[gGameObjectCount].SetScale(1.0f);
	gGameObjects[gGameObjectCount].childrenCount = 0;
	gGameObjects[gGameObjectCount].name = "BoneHandR";

	AttachNodeChild(gPlayerNode->GetChild("BoneForearmR"), &gGameObjects[gGameObjectCount]);

	gGameObjectCount++;

	// axe
	gGameObjects[gGameObjectCount] = GameObject();
	gGameObjects[gGameObjectCount].renderable = true;
	gGameObjects[gGameObjectCount].meshID = MESH_AXE;
	gGameObjects[gGameObjectCount].SetPosition({ 0.0f, 0.0f, 0.0f });
	gGameObjects[gGameObjectCount].SetRotation({ -90.0f, 0.0f, -90.0f });
	gGameObjects[gGameObjectCount].SetScale(1.0f);
	gGameObjects[gGameObjectCount].childrenCount = 0;
	gGameObjects[gGameObjectCount].name = "Axe";

	AttachNodeChild(gPlayerNode->GetChild("BoneHandR"), &gGameObjects[gGameObjectCount]);

	gGameObjectCount++;

	return true;
}

bool gDrawOverlay = true;
uint gMaxChunkMeshesToBuild = 1;

void Game::DoFrame( float dt )
{
	static bool timeLive = true;
	if( input.key[ KEY::NUM_6 ].Down )
	{
		gameTime_.AdvanceTime( dt );
	}
	else if( input.key[ KEY::NUM_4 ].Down )
	{
		gameTime_.DecreaseTime( dt );
	}
	if( input.key[ KEY::NUM_8 ].Down )
	{
		gSunElevation += 1.0f;
	}
	if( input.key[ KEY::NUM_2 ].Down )
	{
		gSunElevation -= 1.0f;
	}
	if( input.key[ KEY::NUM_5 ].Pressed )
	{
		gameTime_.hours = 12;
		gameTime_.minutes = 0;
		gameTime_.seconds = 0;
		gSunElevation = 90.0f;
	}
	if( input.key[ KEY::NUM_0 ].Pressed )
	{
		timeLive = !timeLive;
	}

	if( timeLive )
	{
		gameTime_.AdvanceTime( dt );
	}

	if( input.key[ KEY::NUM_ADD ].Down )
	{
		gPlayerHFOV += 0.5f;
	}
	if( input.key[ KEY::NUM_SUB ].Down )
	{
		gPlayerHFOV -= 0.5f;
	}

	gSunElevation = Clamp( gSunElevation, 0.0f, 90.0f );

	gMaxChunkMeshesToBuild = 1;

	ProfileNewFrame( dt );
	renderer.Begin();

	// measure frame time for display
	static int deltaFramesElapsed;
	static float capturedDelta[ UPDATE_DELTA_FRAMES ];
	static char deltaStr[ 8 ];

	capturedDelta[ deltaFramesElapsed % UPDATE_DELTA_FRAMES ] = dt;
	deltaFramesElapsed++;

	static float sum = 0;
	if( deltaFramesElapsed % UPDATE_DELTA_FRAMES == 0 ) {
		sum = 0;
		for( int i = 0; i < UPDATE_DELTA_FRAMES; i++ )
		{
			sum += capturedDelta[ i ];
		}
	}

	sprintf( deltaStr, "%5.2f", sum / UPDATE_DELTA_FRAMES );

	// player posiion in chunks and blocks
	XMINT3 playerChunkPos = GetPlayerChunkPos( playerPos );
	XMINT3 playerBlockPos = GetPlayerBlockPos( playerPos );

	// Generate chunks around player

	// Invalidate Chunk Cache on demand
	if( input.key[ KEY::ALT ].Down && input.key[ KEY::R ].Pressed )
	{
		for( int z = playerChunkPos.z - gChunkMetaCacheHalfDim; z <= playerChunkPos.z + gChunkMetaCacheHalfDim; z++ )
		{
			for( int x = playerChunkPos.x - gChunkMetaCacheHalfDim; x <= playerChunkPos.x + gChunkMetaCacheHalfDim; x++ )
			{
				int index = ChunkCacheIndexFromChunkPos( x, z, gChunkCacheDim );
				Chunk *chunk = &gChunkCache[ index ];

				chunk->pos[0] = INT_MAX;
				chunk->pos[1] = INT_MAX;
			}
		}

		for( int z = playerChunkPos.z - gChunkMeshCacheHalfDim; z <= playerChunkPos.z + gChunkMeshCacheHalfDim; z++ )
		{
			for( int x = playerChunkPos.x - gChunkMeshCacheHalfDim; x <= playerChunkPos.x + gChunkMeshCacheHalfDim; x++ )
			{
				ChunkMesh *chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( x, z, gChunkMeshCacheDim ) ];
				chunkMesh->dirty = true;
			}
		}
	}

	// clear outdated meta data
	for( int z = playerChunkPos.z - gChunkMetaCacheHalfDim; z <= playerChunkPos.z + gChunkMetaCacheHalfDim; z++ )
	{
		for( int x = playerChunkPos.x - gChunkMetaCacheHalfDim; x <= playerChunkPos.x + gChunkMetaCacheHalfDim; x++ )
		{
			Chunk * meta = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x, z, gChunkMetaCacheDim ) ];

			if( meta->pos[0] == INT_MAX &&
				meta->pos[1] == INT_MAX )
				continue;

			if( !( meta->pos[0] == x && meta->pos[1] == z ) )
			{
				ClearChunk( meta );
				meta->terrainGenerated = false;
				meta->pos[0] = INT_MAX;
				meta->pos[1] = INT_MAX;
			}
		}
	}

	int chunksGenerated = 0;
	for( int z = playerChunkPos.z - gChunkCacheHalfDim; z <= playerChunkPos.z + gChunkCacheHalfDim; z++ )
	{
		for( int x = playerChunkPos.x - gChunkCacheHalfDim; x <= playerChunkPos.x + gChunkCacheHalfDim; x++ )
		{
			int index = ChunkCacheIndexFromChunkPos( x, z, gChunkCacheDim );
			Chunk *chunk = &gChunkCache[ index ];

			if( chunk->pos[0] == x && chunk->pos[1] == z )
			{
				// use stored chunk
			}
			else
			{
				// generate new chunk
				chunksGenerated++;

				Chunk * chunkSamZPosX = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x+1, z, gChunkMetaCacheDim ) ];
				Chunk * chunkSamZNegX = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x-1, z, gChunkMetaCacheDim ) ];
				Chunk * chunkPosZSamX = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x, z+1, gChunkMetaCacheDim ) ];
				Chunk * chunkNegZSamX = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x, z-1, gChunkMetaCacheDim ) ];
				Chunk * chunkPosZPosX = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x+1, z+1, gChunkMetaCacheDim ) ];
				Chunk * chunkPosZNegX = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x-1, z+1, gChunkMetaCacheDim ) ];
				Chunk * chunkNegZPosX = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x+1, z-1, gChunkMetaCacheDim ) ];
				Chunk * chunkNegZNegX = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x-1, z-1, gChunkMetaCacheDim ) ];

				ChunkContext adjacentChunks = ChunkContext( chunkNegZNegX, chunkNegZSamX, chunkNegZPosX,
															chunkSamZNegX, chunk,		  chunkSamZPosX,
															chunkPosZNegX, chunkPosZSamX, chunkPosZPosX );

				GenerateChunk( chunk, x, z );
				GenerateChunkStructures( chunk, adjacentChunks );
			}
		}
	}

	for( int z = playerChunkPos.z - gChunkCacheHalfDim; z <= playerChunkPos.z + gChunkCacheHalfDim; z++ )
	{
		for( int x = playerChunkPos.x - gChunkCacheHalfDim; x <= playerChunkPos.x + gChunkCacheHalfDim; x++ )
		{
			Chunk * chunk = &gChunkCache[ ChunkCacheIndexFromChunkPos( x, z, gChunkCacheDim ) ];
			Chunk * meta  = &gChunkMetaCache[ ChunkCacheIndexFromChunkPos( x, z, gChunkMetaCacheDim ) ];

			// add data from adjacent chunks if any
			if( meta->pos[0] == x &&
				meta->pos[1] == z &&
				chunk->terrainGenerated &&
				meta->terrainGenerated )
			{
				assert( chunk->pos[0] == meta->pos[0] && chunk->pos[1] == meta->pos[1] );
				MergeChunks( chunk, meta );
				meta->terrainGenerated = false;
			}
		}
	}

	// player movement
	float dTSec = dt / 1000.0f;
	XMVECTOR vPlayerPos, vDir, vLook, vRight, vUp, vSpeed, force, acceleration, drag, vGravity;
	vPlayerPos 	= XMLoadFloat3( &playerPos );
	vDir 	= XMLoadFloat3( &playerDir );
	vLook 	= XMLoadFloat3( &playerLook );
	vRight 	= XMLoadFloat3( &playerRight );
	vUp 	= XMLoadFloat3( &playerUp );
	vSpeed 	= XMLoadFloat3( &playerSpeed );

	force = XMVectorZero();
	acceleration = XMVectorZero();
	vGravity = XMLoadFloat3( &gravity ) * playerMass;

	if( input.key[ KEY::W ].Down ) {
		force += vDir;
	}
	if( input.key[ KEY::S ].Down ) {
		force -= vDir;
	}
	if( input.key[ KEY::D ].Down ) {
		force += vRight;
	}
	if( input.key[ KEY::A ].Down ) {
		force -= vRight;
	}

	force = XMVector4Normalize( force ) * 1900.0f;

	if( input.key[ KEY::SPACE ].Down ) {
		if( !playerAirborne ) {
			force += vUp * 2500.0f * playerMass * 2.35f / dt;
			playerAirborne = true;
		}
	}

	if( input.key[ KEY::LSHIFT ].Down ) {
		force *= 5;
	}

	force += vGravity;

	drag = 3.5f * vSpeed;
	drag = XMVectorSetY( drag, XMVectorGetY( drag ) * 0.12f );

	acceleration = force / playerMass - drag;

	// check for world collisions
	XMVECTOR vTargetPos = vPlayerPos + vSpeed * dTSec + ( acceleration * dTSec * dTSec ) / 2.0f;
	XMVECTOR vPlayerDelta = vTargetPos - vPlayerPos;

	XMFLOAT3 targetPos = {};
	XMStoreFloat3( &targetPos, vTargetPos );

	XMINT3 targetChunkPos = GetPlayerChunkPos( targetPos );
	XMINT3 targetBlockPos = GetPlayerBlockPos( targetPos );

	//BLOCK_TYPE targetBT = GetBlockType( world_->chunks[ ChunkCacheIndexFromChunkPos( targetChunkPos.x, targetChunkPos.z ) ], targetBlockPos );

	// add non-air blocks around player to collision testing
#if 0
	AABB boxes[9];
	int boxIndex = 0;
	for( int x = playerBlockPos.x - 1; x <= playerBlockPos.x + 1; x++ )
	{
		for( int y = playerBlockPos.y - 1; y <= playerBlockPos.y + 1; y++ )
		{
			for( int z = playerBlockPos.z - 1; z <= playerBlockPos.z + 1; z++ )
			{
				if( z > 0 && z < CHUNK_WIDTH && x > 0 && x < CHUNK_WIDTH &&
					GetBlockType( world_->chunks[ ChunkCacheIndexFromChunkPos( playerChunkPos.x, playerChunkPos.z ) ], XMINT3( x, y, z ) ) != BT_AIR )
				{
					boxes[ boxIndex ] = { XMFLOAT3( x, y, z ), XMFLOAT3( x+1, y+1, z+1 ) };
					boxIndex++;
				}
			}
		}
	}
#endif

	// decompose playerDelta to three main axes and test for collision per axis
	float playerDeltaX = XMVectorGetX( vPlayerDelta );
	float playerDeltaY = XMVectorGetY( vPlayerDelta );
	float playerDeltaZ = XMVectorGetZ( vPlayerDelta );

	XMFLOAT3 targetPosX = playerPos;
	targetPosX.x += playerDeltaX;

	XMFLOAT3 targetPosY = playerPos;
	targetPosY.y += playerDeltaY;

	XMFLOAT3 targetPosZ = playerPos;
	targetPosZ.z += playerDeltaZ;

	XMINT3 targetChunkPosX = GetPlayerChunkPos( targetPosX );
	XMINT3 targetChunkPosY = GetPlayerChunkPos( targetPosY );
	XMINT3 targetChunkPosZ = GetPlayerChunkPos( targetPosZ );

	XMINT3 targetBlockPosX = GetPlayerBlockPos( targetPosX );
	XMINT3 targetBlockPosY = GetPlayerBlockPos( targetPosY );
	XMINT3 targetBlockPosZ = GetPlayerBlockPos( targetPosZ );

	BLOCK_TYPE targetBlockTypeX = GetBlockType( gChunkCache[ ChunkCacheIndexFromChunkPos(targetChunkPosX.x, targetChunkPosX.z, gChunkCacheDim) ], targetBlockPosX );
	if( targetBlockTypeX != BT_AIR )
	{
		playerDeltaX = 0.0f;
		vSpeed = XMVectorSetX( vSpeed, 0.0f );
	}

	BLOCK_TYPE targetBlockTypeY = GetBlockType( gChunkCache[ ChunkCacheIndexFromChunkPos(targetChunkPosY.x, targetChunkPosY.z, gChunkCacheDim) ], targetBlockPosY );
	if( targetBlockTypeY != BT_AIR && playerSpeed.y < 0.0f )
	{
		playerDeltaY = 0.0f;
		playerAirborne = false;
		vSpeed = XMVectorSetY( vSpeed, 0.0f );
		acceleration = XMVectorSetY( acceleration, -9.8f );
	}

	BLOCK_TYPE targetBlockTypeZ = GetBlockType( gChunkCache[ ChunkCacheIndexFromChunkPos(targetChunkPosZ.x, targetChunkPosZ.z, gChunkCacheDim) ], targetBlockPosZ );
	if( targetBlockTypeZ != BT_AIR )
	{
		playerDeltaZ = 0.0f;
		vSpeed = XMVectorSetZ( vSpeed, 0.0f );
	}

	vTargetPos = XMVectorSet( playerPos.x + playerDeltaX, playerPos.y + playerDeltaY, playerPos.z + playerDeltaZ, 1.0f );


	// move player
	vPlayerPos = vTargetPos;
	vSpeed = vSpeed + acceleration * dTSec;

	float yawDegrees = 0;
	float pitchDegrees = 0;
	static float playerYawDegrees = 0;
	static float playerPitchDegrees = 0;

	if( input.mouse.x ) {
		yawDegrees = input.mouse.x / 10.0f;
		XMMATRIX yaw = XMMatrixRotationY( XMConvertToRadians( yawDegrees ) );
		vDir = XMVector4Transform( vDir, yaw );
		vLook = XMVector4Transform( vLook, yaw );
		vRight = XMVector4Transform( vRight, yaw );
	}
	if( input.mouse.y ) {
		pitchDegrees = input.mouse.y / 10.0f;
		XMMATRIX pitch = XMMatrixRotationAxis( vRight,
											   XMConvertToRadians( pitchDegrees ) );
		XMVECTOR newLook = XMVector4Transform( vLook, pitch );
		// prevent upside-down
		float elevationAngleCos = XMVectorGetX( XMVector4Dot( vDir, newLook ) );
		if( fabsf( elevationAngleCos ) > 0.1 )
		{
			vLook = newLook;

			playerPitchDegrees += pitchDegrees;
		}
	}

	playerYawDegrees += yawDegrees;
	if(playerYawDegrees >= 360.0f)
		playerYawDegrees -= 360.0f;
	else if(playerYawDegrees < 0)
		playerYawDegrees = 360.0f + playerYawDegrees;

	XMMATRIX mPlayerYaw = XMMatrixRotationY( XMConvertToRadians( playerYawDegrees ) );
	XMMATRIX mPlayerPitch = XMMatrixRotationAxis( vRight,
												  XMConvertToRadians( playerPitchDegrees ) );

	XMStoreFloat3( &playerPos, vPlayerPos );
	XMStoreFloat3( &playerDir, vDir );
	XMStoreFloat3( &playerLook, vLook );
	XMStoreFloat3( &playerRight, vRight );
	XMStoreFloat3( &playerUp, vUp );
	XMStoreFloat3( &playerSpeed, vSpeed );

	XMFLOAT3 playerEyePos = playerPos;
	playerEyePos.y += playerHeight;
	XMVECTOR vEyePos = XMLoadFloat3( &playerEyePos );

	// update player skeleton in the scene graph
	gPlayerNode->rotation = mPlayerYaw;
	gPlayerNode->SetPosition({ playerPos.x, playerPos.y, playerPos.z});
	gPlayerNode->GetChild("BoneShoulderR")->SetRotation({ gPlayerNode->GetChild("BoneShoulderR")->transform.rotation.x,
														  gPlayerNode->GetChild("BoneShoulderR")->transform.rotation.y,
														  -playerPitchDegrees*0.6f} );

	// weapon motion
	// TODO: motion is too plain and arbitrary, add detail
	float st = ((float)gameTime_.elapsedMS * 0.0005f);
	float forearmY = (0.5f * (cosf(st) + 0.5f*cosf(3*st + 11) + 0.4*cosf(7*st + 41)));
	float forearmZ = (90.0f + 0.8*cos(st) + 0.6*cos(3*st + 37) + 0.3*cos(5*st + 41));
	gPlayerNode->GetChild("BoneForearmR")->SetRotation({ gPlayerNode->GetChild("BoneForearmR")->transform.rotation.x, forearmY, forearmZ });

	char playerLookDirectionName[3];
	// detect player look direction
	{
		if(fabs(playerLook.x) > fabs(playerLook.y))
		{
			if(fabs(playerLook.z) > fabs(playerLook.x))
			{
				if(playerLook.z >= 0)
				{
					playerLookDirectionName[0] = '+';
					playerLookDirectionName[1] = 'Z';
					playerLookDirectionName[2] = '\0';
				}
				else
				{
					playerLookDirectionName[0] = '-';
					playerLookDirectionName[1] = 'Z';
					playerLookDirectionName[2] = '\0';
				}
			}
			else
			{
				if(playerLook.x >= 0)
				{
					playerLookDirectionName[0] = '+';
					playerLookDirectionName[1] = 'X';
					playerLookDirectionName[2] = '\0';
				}
				else
				{
					playerLookDirectionName[0] = '-';
					playerLookDirectionName[1] = 'X';
					playerLookDirectionName[2] = '\0';
				}
			}
		}
		else
		{
			if(fabs(playerLook.z) > fabs(playerLook.y))
			{
				if(playerLook.z >= 0)
				{
					playerLookDirectionName[0] = '+';
					playerLookDirectionName[1] = 'Z';
					playerLookDirectionName[2] = '\0';
				}
				else
				{
					playerLookDirectionName[0] = '-';
					playerLookDirectionName[1] = 'Z';
					playerLookDirectionName[2] = '\0';
				}
			}
			else
			{
				if(playerLook.y >= 0)
				{
					playerLookDirectionName[0] = '+';
					playerLookDirectionName[1] = 'Y';
					playerLookDirectionName[2] = '\0';
				}
				else
				{
					playerLookDirectionName[0] = '-';
					playerLookDirectionName[1] = 'Y';
					playerLookDirectionName[2] = '\0';
				}
			}
		}
	}



	bool blockPicked = false;
	XMFLOAT3 lookAtTarget;
	XMFLOAT3 pickDirection;
	RayAABBIntersection intersection;
	intersection.point = { 0.0f, 0.0f, 0.0f };
	intersection.plane = { 0.0f, 0.0f, 0.0f };
	BlockPosition pickedBlock = { 0 };
	Line lineOfSight;
	// block picking
	{
		XMVECTOR vPlayerEyePos = XMLoadFloat3( &playerEyePos );
		XMVECTOR vTarget = vPlayerEyePos + vLook * playerReach;
		XMVECTOR vPick = vLook * playerReach;
		XMStoreFloat3( &lookAtTarget, vTarget );
		XMStoreFloat3( &pickDirection, vPick );
		// Segment lineOfSight = { playerEyePos, lookAtTarget };
		lineOfSight = { playerEyePos, pickDirection };

		int chunkX = playerChunkPos.x;
		int chunkZ = playerChunkPos.z;
		Chunk * chunk;
		float lastDistance = 1000.0f;
		int blockX, blockY, blockZ;
		int playerReachBlocks = (int)playerReach + 1;
		for( int blockOffsetX = playerBlockPos.x - playerReachBlocks; blockOffsetX <= playerBlockPos.x + playerReachBlocks; blockOffsetX++ )
		{
			if( blockOffsetX < 0 )
			{
				blockX = blockOffsetX + CHUNK_WIDTH;
				chunkX = playerChunkPos.x - 1;
			}
			else if( blockOffsetX >= CHUNK_WIDTH )
			{
				blockX = blockOffsetX - CHUNK_WIDTH;
				chunkX = playerChunkPos.x + 1;
			}
			else
			{
				blockX = blockOffsetX;
				chunkX = playerChunkPos.x;
			}
			for( int blockOffsetY = playerBlockPos.y - playerReachBlocks; blockOffsetY <= playerBlockPos.y + playerReachBlocks; blockOffsetY++ )
			{
				if( blockOffsetY < 0 || blockOffsetY >= CHUNK_HEIGHT )
				{
					continue;
				}
				else
				{
					blockY = blockOffsetY;
				}
				for( int blockOffsetZ = playerBlockPos.z - playerReachBlocks; blockOffsetZ <= playerBlockPos.z + playerReachBlocks; blockOffsetZ++ )
				{
					if( blockOffsetZ < 0 )
					{
						blockZ = blockOffsetZ + CHUNK_WIDTH;
						chunkZ = playerChunkPos.z - 1;
					}
					else if( blockOffsetZ >= CHUNK_WIDTH )
					{
						blockZ = blockOffsetZ - CHUNK_WIDTH;
						chunkZ = playerChunkPos.z + 1;
					}
					else
					{
						blockZ = blockOffsetZ;
						chunkZ = playerChunkPos.z;
					}
					chunk = &gChunkCache[ ChunkCacheIndexFromChunkPos( chunkX, chunkZ, gChunkCacheDim) ];
					if( GetBlockType( *chunk, { blockX, blockY, blockZ } ) != BT_AIR )
					{

						AABB blockBound = { {
												(float)( chunkX * CHUNK_WIDTH + blockX ),
												(float)( blockY ),
												(float)( chunkZ * CHUNK_WIDTH + blockZ )
											},
											{
												(float)( chunkX * CHUNK_WIDTH + blockX+1.0f ),
												(float)( blockY+1.0f ),
												(float)( chunkZ * CHUNK_WIDTH + blockZ+1.0f )
											} };

						if( TestIntersection( lineOfSight, blockBound ) )
						{
							blockPicked = true;
							float distance = DistanceSq( playerEyePos, { blockBound.min.x + 0.5f, blockBound.min.y + 0.5f, blockBound.min.z + 0.5f } );

							if( distance < lastDistance )
							{
								pickedBlock = { chunkX, chunkZ, blockX, blockY, blockZ };
								lastDistance = distance;
							}
						}
					}
				}
			}
		}

		AABB blockBound = { {
								(float)( pickedBlock.chunkX * CHUNK_WIDTH + pickedBlock.x ),
								(float)( pickedBlock.y ),
								(float)( pickedBlock.chunkZ * CHUNK_WIDTH + pickedBlock.z )
							},
							{
								(float)( pickedBlock.chunkX * CHUNK_WIDTH + pickedBlock.x+1.0f ),
								(float)( pickedBlock.y+1.0f ),
								(float)( pickedBlock.chunkZ * CHUNK_WIDTH + pickedBlock.z+1.0f )
							} };
		intersection = GetIntersection( lineOfSight, blockBound );

	} // block picking

	// block place / remove
	if( blockPicked )
	{
		int adjOffsetX = 0;
		int adjOffsetZ = 0;
		ChunkMesh *chunkMesh = 0;

		if( input.key[ KEY::LMB ].Pressed )
		{
			BlockPosition placedBlock = pickedBlock;
			placedBlock.x += (int)( intersection.plane.x );
			placedBlock.y += (int)( intersection.plane.y );
			placedBlock.z += (int)( intersection.plane.z );
			if( placedBlock.x == -1 ) {
				placedBlock.x = CHUNK_WIDTH - 1;
				placedBlock.chunkX--;
			}
			if( placedBlock.x == CHUNK_WIDTH ) {
				placedBlock.x = 0;
				placedBlock.chunkX++;
			}
			if( placedBlock.z == -1 ) {
				placedBlock.z = CHUNK_WIDTH - 1;
				placedBlock.chunkZ--;
			}
			if( placedBlock.z == CHUNK_WIDTH ) {
				placedBlock.z = 0;
				placedBlock.chunkZ++;
			}

			SetBlockType( &gChunkCache[ ChunkCacheIndexFromChunkPos( placedBlock.chunkX, placedBlock.chunkZ, gChunkCacheDim ) ],
											{
												placedBlock.x,
												placedBlock.y,
												placedBlock.z,
											}, BT_WOOD );
			chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( placedBlock.chunkX, placedBlock.chunkZ, gChunkMeshCacheDim ) ];
			chunkMesh->dirty = true;

			if( placedBlock.x == 0 ) {
				adjOffsetX = -1;
			}
			if( placedBlock.x == CHUNK_WIDTH-1 ) {
				adjOffsetX = +1;
			}
			if( placedBlock.z == 0 ) {
				adjOffsetZ = -1;
			}
			if( placedBlock.z == CHUNK_WIDTH-1 ) {
				adjOffsetZ = +1;
			}

			if( adjOffsetX != 0 )
			{
				chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( placedBlock.chunkX + adjOffsetX, placedBlock.chunkZ, gChunkMeshCacheDim ) ];
				chunkMesh->dirty = true;
				++gMaxChunkMeshesToBuild;
			}
			if( adjOffsetZ != 0 )
			{
				chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( placedBlock.chunkX, placedBlock.chunkZ + adjOffsetZ, gChunkMeshCacheDim ) ];
				chunkMesh->dirty = true;
				++gMaxChunkMeshesToBuild;
			}
			if( adjOffsetX != 0 && adjOffsetZ != 0 )
			{
				chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( placedBlock.chunkX + adjOffsetX, placedBlock.chunkZ + adjOffsetZ, gChunkMeshCacheDim ) ];
				chunkMesh->dirty = true;
				++gMaxChunkMeshesToBuild;
			}

		} // LMB Pressed

		if( input.key[ KEY::RMB ].Pressed )
		{
			SetBlockType( &gChunkCache[ ChunkCacheIndexFromChunkPos( pickedBlock.chunkX, pickedBlock.chunkZ, gChunkCacheDim ) ],
											{
												pickedBlock.x,
												pickedBlock.y,
												pickedBlock.z,
											}, BT_AIR );
			chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( pickedBlock.chunkX, pickedBlock.chunkZ, gChunkMeshCacheDim ) ];
			chunkMesh->dirty = true;

			if( pickedBlock.x == 0 ) {
				adjOffsetX = -1;
			}
			if( pickedBlock.x == CHUNK_WIDTH-1 ) {
				adjOffsetX = +1;
			}
			if( pickedBlock.z == 0 ) {
				adjOffsetZ = -1;
			}
			if( pickedBlock.z == CHUNK_WIDTH-1 ) {
				adjOffsetZ = +1;
			}

			if( adjOffsetX != 0 )
			{
				chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( pickedBlock.chunkX + adjOffsetX, pickedBlock.chunkZ, gChunkMeshCacheDim ) ];
				chunkMesh->dirty = true;
				++gMaxChunkMeshesToBuild;
			}
			if( adjOffsetZ != 0 )
			{
				chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( pickedBlock.chunkX, pickedBlock.chunkZ + adjOffsetZ, gChunkMeshCacheDim ) ];
				chunkMesh->dirty = true;
				++gMaxChunkMeshesToBuild;
			}
			if( adjOffsetX != 0 && adjOffsetZ != 0 )
			{
				chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( pickedBlock.chunkX + adjOffsetX, pickedBlock.chunkZ + adjOffsetZ, gChunkMeshCacheDim ) ];
				chunkMesh->dirty = true;
				++gMaxChunkMeshesToBuild;
			}

		} // RMB Pressed
	} // block place / remove

	// world rendering
//	int batchVertexCount = 0;
//	int numDrawnBatches = 0;
	int numDrawnVertices = 0;
	uint chunkMeshesRebuilt = 0;

	if( input.key[ KEY::F1 ].Pressed ) {
		gDrawOverlay = !gDrawOverlay;
	}

	for( int z = playerChunkPos.z - gChunkMeshCacheHalfDim; z <= playerChunkPos.z + gChunkMeshCacheHalfDim; z++ )
	{
		for( int x = playerChunkPos.x - gChunkMeshCacheHalfDim; x <= playerChunkPos.x + gChunkMeshCacheHalfDim; x++ )
		{
			ChunkMesh *chunkMesh = &gChunkMeshCache[ MeshCacheIndexFromChunkPos( x, z, gChunkMeshCacheDim ) ];
			if( !chunkMesh->dirty && chunkMesh->vertices && chunkMesh->chunkPos[0] == x && chunkMesh->chunkPos[1] == z )
			{
				// use chunk mesh
			}
			else
			{
				// rebuild chunk mesh
				chunkMesh->dirty = false;
				chunkMeshesRebuilt++;

				chunkMesh->chunkPos[0] = x;
				chunkMesh->chunkPos[1] = z;

				if( chunkMesh->vertices ) {
					delete[] chunkMesh->vertices;
					chunkMesh->vertices = 0;
				}

				// TODO: any reason to break here?
				if( chunkMeshesRebuilt > gMaxChunkMeshesToBuild ) {
					break;
				}

				Chunk* chunk = &gChunkCache[ ChunkCacheIndexFromChunkPos( x, z, gChunkCacheDim ) ];

				Chunk* chunkSamZPosX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x+1, z, gChunkCacheDim ) ];
				Chunk* chunkSamZNegX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x-1, z, gChunkCacheDim ) ];
				Chunk* chunkPosZSamX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x, z+1, gChunkCacheDim ) ];
				Chunk* chunkNegZSamX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x, z-1, gChunkCacheDim ) ];

				Chunk* chunkPosZPosX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x+1, z+1, gChunkCacheDim ) ];
				Chunk* chunkPosZNegX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x-1, z+1, gChunkCacheDim ) ];
				Chunk* chunkNegZPosX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x+1, z-1, gChunkCacheDim ) ];
				Chunk* chunkNegZNegX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x-1, z-1, gChunkCacheDim ) ];

				ChunkContext adjacentChunks = ChunkContext( chunkNegZNegX, chunkNegZSamX, chunkNegZPosX,
															chunkSamZNegX, chunk,		  chunkSamZPosX,
															chunkPosZNegX, chunkPosZSamX, chunkPosZPosX );

#if 0
				GenerateChunkMesh( chunkMesh, chunkNegXPosZ,	chunkPosZ,	chunkPosXPosZ,
											  chunkNegX,		chunk,		chunkPosX,
											  chunkNegXNegZ,	chunkNegZ,	chunkPosXNegZ );
#else
				GenerateChunkMesh( chunkMesh, adjacentChunks );
#endif
				int meshIndex = MeshCacheIndexFromChunkPos( x, z, gChunkMeshCacheDim );

				// renderer.SubmitChunkMesh( MeshCacheIndexFromChunkPos( x, z, gChunkMeshCacheDim ), chunkMesh->vertices, chunkMesh->numVertices );
				gChunkVertexBuffer->Release( meshIndex );
				gChunkVertexBuffer->Create( meshIndex,
											sizeof( BlockVertex ),
											chunkMesh->numVertices,
											renderer.GetDevice(),
											chunkMesh->vertices );
			} // else

		} // for chunkX
		if( chunkMeshesRebuilt > gMaxChunkMeshesToBuild ) {
			break;
		}
	} // for chunkZ

	//delete[] vertexBuffer;

	if( chunkMeshesRebuilt != 0 )
	{
		OutputDebugStringA( std::to_string( chunkMeshesRebuilt ).c_str() );
		OutputDebugStringA( " chunk meshes rebuilt\n" );
	}

	if( chunksGenerated != 0 )
	{
		OutputDebugStringA( std::to_string( chunksGenerated ).c_str() );
		OutputDebugStringA( " chunks generated\n" );
	}

//	if( batchVertexCount ) {
//		renderer.Draw( batchVertexCount );
//		numDrawnBatches++;
//	}

	ProfileStart( "Render" );
	// draw shadow map

	// Render state for shadow drawing
	renderer.SetChunkDrawingState();
	renderer.SetDepthStencilState( gDefaultDepthStencilState );
	renderer.SetSampler( SAMPLER_POINT, ST_FRAGMENT, 1 );
	renderer.SetShader( SHADER_SHADOWMAP );
	renderer.RemoveTexture( ST_FRAGMENT, 1 );
	//renderer.RemoveTexture( ST_FRAGMENT, 5 );
	//renderer.RemoveTexture( ST_FRAGMENT, 6 );
	//renderer.RemoveTexture( ST_FRAGMENT, 7 );

	D3D11_VIEWPORT smViewport;
	smViewport.Width = SM_RESOLUTION;
	smViewport.Height = SM_RESOLUTION;
	smViewport.MinDepth = 0.0f;
	smViewport.MaxDepth = 1.0f;
	smViewport.TopLeftX = 0.0f;
	smViewport.TopLeftY = 0.0f;
	renderer.SetViewport( &smViewport );

	// time of day
	// t: [ 0, 86400 )
	float t =	gameTime_.hours * 60.0f * 60.0f +
				gameTime_.minutes * 60.0f +
				gameTime_.seconds;
	t /= 86400.0f;

	// sun zenith angle, 0..2PI
	//  0, PI - sun at horizon
	//  1..PI - day, PI..2PI - night
	float sunAzimuth = t * XM_PI * 2.0f - XM_PI * 0.5f;
	float sunElevation = XMConvertToRadians( gSunElevation );

	XMFLOAT3 sunDirection;
	XMFLOAT3 negSunDirection;
	XMFLOAT3 sunRight;
	XMFLOAT3 sunUp;

	sunDirection.y = sin( sunAzimuth ) * cos( XM_PIDIV2 - sunElevation );
	sunDirection.z = sin( sunAzimuth ) * sin( XM_PIDIV2 - sunElevation );
	sunDirection.x = cos( sunAzimuth );

	float smRegionDim = (float)gChunkMeshCacheDim * 0.5f * CHUNK_WIDTH;

	ProfileStart( "ShadowmapPass" );
	XMVECTOR vSunDirection = -XMVector4Normalize( XMLoadFloat3( &sunDirection ) );
	XMVECTOR vLightUp = XMVector4Normalize( XMVectorSet( 0.0f, cos( sunElevation ), -sin( sunElevation ), 0.0f ) );
	XMVECTOR vLightRight = XMVector4Normalize( XMVector3Cross( vLightUp, vSunDirection ) );
	XMMATRIX lightView = XMMatrixLookToLH( XMVectorZero(), vSunDirection, vLightUp );
	XMMATRIX invLightView = XMMatrixTranspose( lightView );

	XMStoreFloat3( &sunDirection, -vSunDirection );
	XMStoreFloat3( &negSunDirection, vSunDirection );
	XMStoreFloat3( &sunRight, vLightRight );
	XMStoreFloat3( &sunUp, vLightUp );

	// player frustum
	float screenAspect = (float)renderer.GetViewportWidth() / (float)renderer.GetViewportHeight();
	XMVECTOR vLookUp = XMVector3Cross( vLook, vRight );
	XMFLOAT3 lookUp;
	XMStoreFloat3( &lookUp, vLookUp );

	Frustum playerFrustum = CalculatePerspectiveFrustum( playerEyePos, playerLook, playerRight, lookUp, gPlayerNearPlane, gPlayerFarPlane, gPlayerHFOV, screenAspect );

	// frustum slices
	float sliceDistances[ gNumShadowCascades + 1 ];
	sliceDistances[0] = gPlayerNearPlane;
	for( int i = 1; i < gNumShadowCascades; ++i )
	{
		float t = 0.96f;
		float linearSplit = gPlayerNearPlane + (i / (float)gNumShadowCascades)*(gPlayerFarPlane - gPlayerNearPlane);
		float expSplit = gPlayerNearPlane * pow( ( gPlayerFarPlane / gPlayerNearPlane ), i / (float)gNumShadowCascades );
		sliceDistances[i] = Lerp( linearSplit, expSplit, t );
	}
	sliceDistances[gNumShadowCascades] = gPlayerFarPlane;

	int numChunksToDrawSM = gChunkMeshCacheDim * gChunkMeshCacheDim * gNumShadowCascades;
	int numChunksDrawnSM = 0;

	LightCB lightCBData[4];

	Frustum sunFrustum[4];

	float smSize[4];

	renderer.ClearTexture( &gShadowDB );

	for( int sliceIndex = 0; sliceIndex < gNumShadowCascades; ++sliceIndex )
	{
		float sliceNear = sliceDistances[sliceIndex];
		float sliceFar = sliceDistances[sliceIndex+1];
		Frustum playerFrustumSlice = CalculatePerspectiveFrustum( playerEyePos, playerLook, playerRight, lookUp,
																  sliceNear, sliceFar, gPlayerHFOV, screenAspect );

		// find bound sphere center
		// TODO: calculate a tighter sphere
		XMFLOAT3 playerFrustumSliceCenter;
		XMVECTOR vPlayerFrustumSliceCenter = vEyePos + vLook * sliceNear + vLook * (( sliceFar - sliceNear ) * 0.5f);
		// vPlayerFrustumSliceCenter = vEyePos;
		XMStoreFloat3( &playerFrustumSliceCenter, vPlayerFrustumSliceCenter );

		XMFLOAT3 smBoundCenter;
		XMVECTOR vLightSpaceCenter = XMVector4Transform( XMLoadFloat3( &playerFrustumSliceCenter ), lightView );
		XMStoreFloat3( &smBoundCenter, vLightSpaceCenter );

		// calculate minimum bound sphere radius
		float boundSphereRadius = 0;
		for( int i = 0; i < 8; i++ )
		{
			XMVECTOR vLightSpaceCorner = XMVector4Transform( XMLoadFloat3( &playerFrustumSlice.corners[i] ), lightView );

			if( XMVectorGetX( XMVector3Length( vLightSpaceCorner - vLightSpaceCenter ) ) > boundSphereRadius )
			{
				boundSphereRadius = XMVectorGetX( XMVector3Length( vLightSpaceCorner - vLightSpaceCenter ) );
			}
		}

		XMFLOAT3 smBoundOffset = { boundSphereRadius, boundSphereRadius, smRegionDim*0.5f };

		XMFLOAT3 smBoundMax = smBoundCenter + smBoundOffset;
		XMFLOAT3 smBoundMin = smBoundCenter - smBoundOffset;

		// snap SM bound to texel grid
		float texelsPerMeter = SM_RESOLUTION / (smBoundMax.x-smBoundMin.x);
		smBoundMin = smBoundMin * texelsPerMeter;
		smBoundMax = smBoundMax * texelsPerMeter;
		smBoundMin = Floor( smBoundMin );
		smBoundMax = Floor( smBoundMax );
		smBoundMin = smBoundMin / texelsPerMeter;
		smBoundMax = smBoundMax / texelsPerMeter;

		smSize[ sliceIndex ] = smBoundMax.x - smBoundMin.x;

		XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH( smBoundMin.x,
															  smBoundMax.x,
															  smBoundMin.y,
															  smBoundMax.y,
															  smBoundMin.z,
															  smBoundMax.z );
		XMMATRIX lightVP = XMMatrixTranspose( XMMatrixMultiply( lightView, lightProj ) );

		XMStoreFloat4x4( &lightCBData[ sliceIndex ].vp, lightVP );

		// calculate world-space shadow bound for culling
		sunFrustum[sliceIndex] = ComputeOrthoFrustum( playerFrustumSliceCenter, negSunDirection, sunRight, sunUp,
												  boundSphereRadius*2.0f, boundSphereRadius*2.0f, -smRegionDim*0.5f, smRegionDim*0.5f );

		renderer.SetRasterizerState( gSMRasterizerState[ sliceIndex ] );
		renderer.SetLightCBuffer( lightCBData[ sliceIndex ] );
		//renderer.ClearTexture( &gShadowDB[ sliceIndex ] );
//		renderer.SetRenderTarget( 0, &gShadowDB[ sliceIndex ], sliceIndex );
		renderer.SetRenderTarget( 0, &gShadowDB, sliceIndex );

		for( int meshIndex = 0; meshIndex < gChunkMeshCacheDim * gChunkMeshCacheDim; meshIndex++ )
		{
			AABB chunkBound;
			chunkBound.min = {
				float( gChunkMeshCache[ meshIndex ].chunkPos[0] * CHUNK_WIDTH ),
				0,
				float( gChunkMeshCache[ meshIndex ].chunkPos[1] * CHUNK_WIDTH ) };
			chunkBound.max = {
				chunkBound.min.x + CHUNK_WIDTH,
				CHUNK_HEIGHT,
				chunkBound.min.z + CHUNK_WIDTH };

			bool culled = IsFrustumCulled( sunFrustum[sliceIndex], chunkBound );

			if( gChunkMeshCache[ meshIndex ].vertices && !culled )
			{
				renderer.DrawVertexBuffer(  gChunkVertexBuffer,
											meshIndex,
											gChunkMeshCache[ meshIndex ].chunkPos[0] * CHUNK_WIDTH,
											gChunkMeshCache[ meshIndex ].chunkPos[1] * CHUNK_WIDTH );
				numDrawnVertices += gChunkMeshCache[ meshIndex ].numVertices;
				++numChunksDrawnSM;
			}
		}
	}


	ProfileStop(); // ShadowmapPass

#if 1
	// draw to frame buffer
	// Frame CB
	renderer.SetChunkDrawingState();

	FrameCB cbData;

	float verticalFOV = XMConvertToRadians( gPlayerHFOV ) / screenAspect;
	playerProj = XMMatrixPerspectiveFovLH( verticalFOV, screenAspect, gPlayerNearPlane, gPlayerFarPlane );
	playerView =  XMMatrixLookToLH( vEyePos, vLook, vUp );
	XMMATRIX vp = XMMatrixTranspose( XMMatrixMultiply( playerView, playerProj ) );

	renderer.SetView( playerEyePos, playerLook, playerUp );

	static float debugCameraHeight = 100.0f;
	static float debugCameraViewHeight = 100.0f;

	if( input.key[ KEY::RBRACKET ].Down )
	{
		debugCameraViewHeight += 1.0f;
	}
	if( input.key[ KEY::LBRACKET ].Down )
	{
		debugCameraViewHeight -= 1.0f;
	}

	XMMATRIX debugTopView = XMMatrixLookToLH( vEyePos + vUp*debugCameraHeight, -vUp, vLook );
	// XMMATRIX debugTopProjection = XMMatrixPerspectiveFovLH( XM_PI / 4, screenAspect, 0.01f, 150.0f );
	XMMATRIX debugTopProjection = XMMatrixOrthographicLH( debugCameraViewHeight * screenAspect, debugCameraViewHeight, 1.0f, debugCameraHeight + 50.0f );
	XMMATRIX debugVP = XMMatrixTranspose( XMMatrixMultiply( debugTopView, debugTopProjection ) );

	static bool useDebugProjection = false;
	if( input.key[ KEY::TAB ].Pressed ) {
		useDebugProjection = !useDebugProjection;
	}
	if( useDebugProjection )
	{
		XMStoreFloat4x4( &cbData.vp, debugVP );
	}
	else
	{
		XMStoreFloat4x4( &cbData.vp, vp );
	}
//	XMStoreFloat4x4( &cbData.vp, lightVP );

	cbData.sunColor = { gSunColor.x * Saturate( sunDirection.y ),
						gSunColor.x * Saturate( sunDirection.y ),
						gSunColor.x * Saturate( sunDirection.y ),
						1.0f };
	cbData.ambientColor = { gAmbientColor.x * Clamp( sunDirection.y, 0.2f, 1.0f ),
							gAmbientColor.y * Clamp( sunDirection.y, 0.2f, 1.0f ),
							gAmbientColor.z * Clamp( sunDirection.y, 0.2f, 1.0f ),
							1.0f };

	cbData.sunDirection = XMFLOAT4( sunDirection.x,
									sunDirection.y,
									sunDirection.z,
									0.0f );

	cbData.dayTimeNorm = t;

	cbData.lightVP[0] = lightCBData[0].vp;
	cbData.lightVP[1] = lightCBData[1].vp;
	cbData.lightVP[2] = lightCBData[2].vp;
	cbData.lightVP[3] = lightCBData[3].vp;

	cbData.smTexelSize[0] = smSize[0] / SM_RESOLUTION;
	cbData.smTexelSize[1] = smSize[1] / SM_RESOLUTION;
	cbData.smTexelSize[2] = smSize[2] / SM_RESOLUTION;
	cbData.smTexelSize[3] = smSize[3] / SM_RESOLUTION;

	renderer.SetFrameCBuffer( cbData );


	renderer.SetViewport( 0 );

//	// set square viewport to render from the sun
//	D3D11_VIEWPORT debugViewport;
//	smViewport.Width = 600;
//	smViewport.Height = 600;
//	smViewport.MinDepth = 0.0f;
//	smViewport.MaxDepth = 1.0f;
//	smViewport.TopLeftX = 0.0f;
//	smViewport.TopLeftY = 0.0f;
//	renderer.SetViewport( &debugViewport );

	renderer.SetDepthStencilState( gDefaultDepthStencilState );
	renderer.SetRasterizerState( gDefaultRasterizerState );
	renderer.SetShader( SHADER_BLOCK );
	renderer.SetRenderTarget();

	// set shadow maps
#if 0
	for( int i = 0; i < gNumShadowCascades; ++i )
	{
		renderer.SetTexture( gShadowDB, ST_FRAGMENT, 4 + i );
	}
#else
	renderer.SetTexture( gShadowDB, ST_FRAGMENT, 1 );
#endif

	int numChunksToDrawRT = gChunkMeshCacheDim * gChunkMeshCacheDim;
	int numChunksDrawnRT = 0;

	ProfileStart( "BeautyPass" );
	for( int meshIndex = 0; meshIndex < gChunkMeshCacheDim * gChunkMeshCacheDim; meshIndex++ )
	{
		if( gChunkMeshCache[ meshIndex ].vertices )
		{
			AABB chunkBound;
			chunkBound.min = {
				float( gChunkMeshCache[ meshIndex ].chunkPos[0] * CHUNK_WIDTH ),
				0,
				float( gChunkMeshCache[ meshIndex ].chunkPos[1] * CHUNK_WIDTH ) };
			chunkBound.max = {
				chunkBound.min.x + CHUNK_WIDTH,
				CHUNK_HEIGHT,
				chunkBound.min.z + CHUNK_WIDTH };

			ProfileStart( "Culling" );
			bool culled = IsFrustumCulled( playerFrustum, chunkBound );
			ProfileStop();

			ProfileStart( "BeatyDrawing" );
			if( !culled )
			{
				renderer.DrawVertexBuffer(  gChunkVertexBuffer,
											meshIndex,
											gChunkMeshCache[ meshIndex ].chunkPos[0] * CHUNK_WIDTH,
											gChunkMeshCache[ meshIndex ].chunkPos[1] * CHUNK_WIDTH );
				numDrawnVertices += gChunkMeshCache[ meshIndex ].numVertices;
				numChunksDrawnRT++;
			}
			ProfileStop();

		}
	}
	ProfileStop();

#endif
	ProfileStop();


#if 0
	// temp move held tool
	XMFLOAT3 toolLocalOffset1 = XMFLOAT3(0.2f, -0.3f, 0.5f);// + playerLook*0.47f;// + playerRight*0.23f;
	XMMATRIX toolOffset1 = XMMatrixTranslation( toolLocalOffset1.x, toolLocalOffset1.y, toolLocalOffset1.z );
	// XMMATRIX toolOffset2 = XMMatrixTranslation( playerPos.x, playerPos.y, playerPos.z );
	XMMATRIX toolOffset2 = XMMatrixTranslation( playerEyePos.x, playerEyePos.y, playerEyePos.z );

	float angleH = AngleBetweenNormals(XMFLOAT3(1.0f, 0.0f, 0.0f), Normalize(XMFLOAT3(playerLook.x, 0.0f, playerLook.z)));
	if(playerLook.z > 0)
	{
		angleH = -angleH;
	}
	float angleZ = AngleBetweenNormals(playerLook, Normalize(XMFLOAT3(playerLook.x, 0.0f, playerLook.z)));
	gGameObjects[1].transform.rotation.y = XMConvertToDegrees(angleH) - 90.0f;
	gGameObjects[1].transform.rotation.x = XMConvertToDegrees(angleZ);
#endif

	// propagate hierarchical transform
	UpdateNode(&gGameObjects[0], XMMatrixIdentity());

	// Draw game objects
	for( uint i = 0; i < gGameObjectCount; i++ )
	{
		if(!gGameObjects[i].renderable)
			continue;

		// TODO: replace
		Mesh * mesh = GetMeshByID( gGameObjects[i].meshID );
		renderer.SetShader( mesh->material.shaderID );
		renderer.SetTexture( mesh->material.textureID, ST_FRAGMENT, 0 );
		renderer.SetVertexBuffer( mesh->vertexBufferID );
		// TODO: generic constant buffer handling

		GameObjectCB gameObjectCBData;
		XMStoreFloat4x4( &gameObjectCBData.world, XMMatrixTranspose(gGameObjects[i].world) );
		XMStoreFloat4x4( &gameObjectCBData.rotation, XMMatrixTranspose(gGameObjects[i].rotationNormalMap) );
		gameObjectCBData.translation = gGameObjects[i].transform.position;
		gameObjectCBData.rotationEuler = gGameObjects[i].transform.rotation;
		gameObjectCBData.scale = gGameObjects[i].transform.scale;

		renderer.SetGameObjectCBuffer( gameObjectCBData );

		renderer.Draw( mesh->vertexCount );
	}

	CROSSHAIR_STATE chState = CROSSHAIR_STATE::INACTIVE;
	if( blockPicked )
	{
		chState = CROSSHAIR_STATE::ACTIVE;
		overlay.OulineBlock( 	pickedBlock.chunkX,
								pickedBlock.chunkZ,
								pickedBlock.x,
								pickedBlock.y,
								pickedBlock.z );
	}

	overlay.DrawCrosshair( (int)( renderer.GetViewportWidth()*0.5f ),
						   (int)( renderer.GetViewportHeight()*0.5f ),
						   chState );

	if( gDrawOverlay )
	{
		ProfileStart( "Overlay" );

		float delta = sum / UPDATE_DELTA_FRAMES;
		int fps = (int)(1000 / delta);

		overlay.Reset();
		overlay.WriteLine( "Frame time: %5.2f (%i fps)", sum / UPDATE_DELTA_FRAMES, fps);
		overlay.WriteLine( "Game  time: %i.%i.%i %i:%i:%i", gameTime_.day,
															gameTime_.month,
															gameTime_.year,
															gameTime_.hours,
															gameTime_.minutes,
															(int)gameTime_.seconds );
//		overlay.WriteLine( "Chunk buffer size: %i KB", sizeof( BlockVertex ) * MAX_VERTS_PER_BATCH / 1024 );
//		overlay.WriteLine( "Batches rendered: %i", renderer.numBatches_ );
//		overlay.WriteLine( "Vertices rendered: %i", numDrawnVertices );
//		overlay.WriteLine( "Chunks generated: %i", chunksGenerated );
//		overlay.WriteLine( "Chunk meshes rebuild: %i", chunkMeshesRebuilt );
//		overlay.WriteLine( "Mouse offset: %+03i - %+03i", input.mouse.x, input.mouse.y );
		overlay.WriteLine( "" );
		overlay.WriteLine( "Player pos: %5.2f %5.2f %5.2f (%s)", playerPos.x, playerPos.y, playerPos.z, playerLookDirectionName );
		overlay.WriteLine( "Chunk pos:  %5i ----- %5i", playerChunkPos.x, playerChunkPos.z );
		overlay.WriteLine( "Yaw: %4.1f Pitch: %4.1f", playerYawDegrees, playerPitchDegrees );
//		overlay.WriteLine( "Splits:  %5.2f - %5.2f - %5.2f - %5.2f - %5.2f", sliceDistances[0], sliceDistances[1], sliceDistances[2], sliceDistances[3], sliceDistances[4] );
//		overlay.WriteLine( "SM texel:  %5.2f, %5.2f, %5.2f, %5.2f", cbData.smTexelSize[0], cbData.smTexelSize[1], cbData.smTexelSize[2], cbData.smTexelSize[3] );
//		overlay.WriteLine( "Radius:  %8.6f", smBoundRadius );
//		overlay.WriteLine( "Diagonal:  %8.6f", boundSphereRadius );
//		overlay.WriteLine( "Speed:  %5.2f", XMVectorGetX( XMVector4Length( vSpeed ) ) );
//		overlay.WriteLine( "FOV:  %5.2f", gPlayerHFOV );
		overlay.Write( "Keys pressed: " );
		for( int i = 0; i < KEY::COUNT; i++ )
		{
			if( input.key[i].Down )
			{
				overlay.Write( "%s, ", GetKeyName( (KEY)i ) );
			}
		}

		overlay.WriteLine( "" );
		overlay.WriteLine( "SM: V%3i C%3i T%3i", numChunksDrawnSM,
												 numChunksToDrawSM - numChunksDrawnSM,
												 numChunksToDrawSM );
		overlay.WriteLine( "RT: V%3i C%3i T%3i", numChunksDrawnRT,
												 numChunksToDrawRT - numChunksDrawnRT,
												 numChunksToDrawRT );


		ProfileStop();

#if 0
		// show lines at chunk borders
		for( int x = -1; x <= 1; x++  )
		{
			for( int z = -1; z <= 1; z++  )
			{
				float lineX = (float)( CHUNK_WIDTH * playerChunkPos.x + CHUNK_WIDTH * x);
				float lineZ = (float)( CHUNK_WIDTH * playerChunkPos.z + CHUNK_WIDTH * z);
				overlay.DrawLine( {lineX, 0.0f, lineZ}, {lineX, 255.0f, lineZ}, {1.0f, 0.2f, 0.2f, 1.0f} );
			}
		}
#endif

		// Draw frusta
		overlay.DrawFrustum( playerFrustum, { 0.0f, 0.5f, 1.0f, 1.0f } );
		//overlay.DrawFrustum( playerFrustumSlice, { 1.0f, 0.5f, 0.0f, 1.0f } );

		overlay.DrawFrustum( sunFrustum[0], { 1.0f, 0.0f, 0.0f, 1.0f } );
		overlay.DrawFrustum( sunFrustum[1], { 0.0f, 1.0f, 0.0f, 1.0f } );
		overlay.DrawFrustum( sunFrustum[2], { 0.0f, 0.0f, 1.0f, 1.0f } );
		overlay.DrawFrustum( sunFrustum[3], { 1.0f, 1.0f, 0.0f, 1.0f } );

		if( blockPicked )
		{
			overlay.DrawPoint( intersection.point, { 0.0f, 1.0f, 1.0f, 1.0f } );
		}
		// overlay.DisplayText( renderer.GetViewportWidth()*0.5f, renderer.GetViewportHeight()*0.5f, "+", { 1.0f, 1.0f, 1.0f, 0.1f } );

//		XMFLOAT3 lightPos;
//		XMStoreFloat3( &lightPos, vLightPos );
//		overlay.DrawPoint( lightPos, { 1.0f, 0.0f, 1.0f, 1.0f } );
	}

	renderer.End();

	for( int i = 0; i < KEY::COUNT; i++ )
	{
		input.key[i].Pressed = false;
		input.key[i].Released = false;
	}
	input.mouse.x = 0;
	input.mouse.y = 0;
}

}
