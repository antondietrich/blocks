#include "game.h"

namespace Blocks
{

using namespace DirectX;

// player vars
XMFLOAT3 playerPos 		= { 0.0f, 60.0f, 0.0f };
XMFLOAT3 playerDir 		= { 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerLook 	= { 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerRight 	= { 1.0f,  0.0f, 0.0f };
XMFLOAT3 playerUp 		= { 0.0f,  1.0f, 0.0f };
XMFLOAT3 playerSpeed 	= { 0.0f,  0.0f, 0.0f };
XMFLOAT3 gravity 		= { 0.0f, -9.8f, 0.0f };

XMMATRIX playerView;
XMMATRIX playerProj;

float playerMass = 75.0f;
float playerHeight = 1.8f;
float playerReach = 4.0f;
bool  playerAirborne = true;

float gPlayerHFOV = 80.0f;
float gPlayerNearPlane = 0.1f;
float gPlayerFarPlane = 1.0f;


bool Game::isInstantiated_ = false;

// caches
int gChunkCacheHalfDim = 0;
int gChunkCacheDim = 0;
int gChunkMeshCacheHalfDim = 0;
int gChunkMeshCacheDim = 0;
Chunk * gChunkCache; // [ CHUNK_CACHE_DIM * CHUNK_CACHE_DIM ]
ChunkMesh * gChunkMeshCache;

RenderTarget gShadowRT;
DepthBuffer gShadowDB;

XMFLOAT3 gSunDirection;
XMFLOAT4 gAmbientColor;
XMFLOAT4 gSunColor;
float gSunElevation = 90.0f;

#define SM_RESOLUTION 4096
//#define SM_REGION_DIM 128.0f
//#define SM_REGION_HALF_DEPTH ( VIEW_DISTANCE * CHUNK_WIDTH )

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

}

bool Game::Start( HWND wnd )
{
	for( int i = 0; i < KEY::COUNT; i++ )
	{
		input.key[i] = { 0 };
	}
	input.mouse = {0, 0};

	gameTime_ = { 2015, 1, 1, 12, 0, 0.0f, 60.f };

	gPlayerFarPlane = float(Config.viewDistanceChunks * CHUNK_WIDTH);

	gChunkCacheHalfDim = Config.viewDistanceChunks + 1;
	gChunkCacheDim = gChunkCacheHalfDim * 2 + 1;
	gChunkMeshCacheHalfDim = Config.viewDistanceChunks;
	gChunkMeshCacheDim = gChunkMeshCacheHalfDim * 2 + 1;

	gChunkCache = new Chunk[ gChunkCacheDim * gChunkCacheDim ];
	gChunkMeshCache = new ChunkMesh[ gChunkMeshCacheDim * gChunkMeshCacheDim ];
	// renderer.InitChunkMeshCache( gChunkMeshCacheDim );

	if( !renderer.Start( wnd ) )
	{
		return false;
	}

	if( !overlay.Start( &renderer ) )
	{
		return false;
	}

	InitWorldGen();
	PrefillChunkCache( gChunkCache, gChunkCacheDim );

	for( int i = 0; i < gChunkMeshCacheDim * gChunkMeshCacheDim; i++ ) {
		gChunkMeshCache[i].dirty = true;
		gChunkMeshCache[i].vertices = 0;
		gChunkMeshCache[i].numVertices = 0;
		gChunkMeshCache[i].chunkPos[0] = 0;
		gChunkMeshCache[i].chunkPos[1] = 0;
	}

	if( !gShadowRT.Init( SM_RESOLUTION, SM_RESOLUTION, DXGI_FORMAT_R32_FLOAT, true, renderer.GetDevice() ) )
	{
		return false;
	}
	if( !gShadowDB.Init( SM_RESOLUTION, SM_RESOLUTION, DXGI_FORMAT_D32_FLOAT, 1, 0, false, renderer.GetDevice() ) )
	{
		return false;
	}

	gSunDirection = Normalize( { 0.5f, 0.8f, 0.25f } );
	gAmbientColor = { 0.5f, 0.6f, 0.75f, 1.0f };
	gSunColor = 	{ 1.0f, 0.9f, 0.8f, 1.0f };

	playerProj = XMMatrixPerspectiveFovLH( XMConvertToRadians( 60.0f ), (float)Config.screenWidth / (float)Config.screenHeight, 0.1f, 1000.0f );

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
				GenerateChunk( chunk, x, z );
			}
		}
	}

//	gravity.y = -9.8f;

	// player movement
	float dTSec = dt / 1000.0f;
	XMVECTOR vPos, vDir, vLook, vRight, vUp, vSpeed, force, acceleration, drag, vGravity;
	vPos 	= XMLoadFloat3( &playerPos );
	vDir 	= XMLoadFloat3( &playerDir );
	vLook 	= XMLoadFloat3( &playerLook );
	vRight 	= XMLoadFloat3( &playerRight );
	vUp 	= XMLoadFloat3( &playerUp );
	vSpeed 	= XMLoadFloat3( &playerSpeed );

	force = XMVectorZero();
	acceleration = XMVectorZero();
	// gravity = XMVectorSet( 0.0f, -9.8f, 0.0f, 0.0f ) * playerMass;
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
//	if( input.key[ KEY::LCTRL ].Pressed ) {
//		if( fabs( gravity.y - 0.0f ) < 0.000001f )
//		{
//			gravity.y = -9.8f;
//		}
//		else
//		{
//			gravity.y = 0.0f;
//		}
//	}

	force = XMVector4Normalize( force ) * 1500.0f;

	if( input.key[ KEY::SPACE ].Down ) {
		if( !playerAirborne ) {
			force += vUp * 2500.0f * playerMass * 2.35f / dt;
			playerAirborne = true;
		}
	}

//	if( input.key[ KEY::LSHIFT ].Down ) {
//		force *= 10;
//	}

	force += vGravity;

	drag = 3.5f * vSpeed;
	drag = XMVectorSetY( drag, XMVectorGetY( drag ) * 0.12f );

	acceleration = force / playerMass - drag;

	// check for world collisions
	XMVECTOR vTargetPos = vPos + vSpeed * dTSec + ( acceleration * dTSec * dTSec ) / 2.0f;
	XMVECTOR vPlayerDelta = vTargetPos - vPos;

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
	vPos = vTargetPos;
	vSpeed = vSpeed + acceleration * dTSec;

	if( input.mouse.x ) {
		float yawDegrees = input.mouse.x / 10.0f;
		XMMATRIX yaw = XMMatrixRotationY( XMConvertToRadians( yawDegrees ) );
		vDir = XMVector4Transform( vDir, yaw );
		vLook = XMVector4Transform( vLook, yaw );
		vRight = XMVector4Transform( vRight, yaw );
	}
	if( input.mouse.y ) {
		float pitchDegrees = input.mouse.y / 10.0f;
		XMMATRIX pitch = XMMatrixRotationAxis( vRight,
											   XMConvertToRadians( pitchDegrees ) );
		XMVECTOR newLook = XMVector4Transform( vLook, pitch );
		// prevent upside-down
		float elevationAngleCos = XMVectorGetX( XMVector4Dot( vDir, newLook ) );
		if( fabsf( elevationAngleCos ) > 0.1 )
		{
			vLook = newLook;
		}
	}

	XMStoreFloat3( &playerPos, vPos );
	XMStoreFloat3( &playerDir, vDir );
	XMStoreFloat3( &playerLook, vLook );
	XMStoreFloat3( &playerRight, vRight );
	XMStoreFloat3( &playerUp, vUp );
	XMStoreFloat3( &playerSpeed, vSpeed );

	XMFLOAT3 playerEyePos = playerPos;
	playerEyePos.y += playerHeight;
	XMVECTOR vEyePos = XMLoadFloat3( &playerEyePos );

	// renderer.SetView( playerEyePos, playerLook, playerUp );

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

				Chunk* chunkPosX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x+1, z, gChunkCacheDim ) ];
				Chunk* chunkNegX = &gChunkCache[ ChunkCacheIndexFromChunkPos( x-1, z, gChunkCacheDim ) ];
				Chunk* chunkPosZ = &gChunkCache[ ChunkCacheIndexFromChunkPos( x, z+1, gChunkCacheDim ) ];
				Chunk* chunkNegZ = &gChunkCache[ ChunkCacheIndexFromChunkPos( x, z-1, gChunkCacheDim ) ];

				Chunk* chunkPosXPosZ = &gChunkCache[ ChunkCacheIndexFromChunkPos( x+1, z+1, gChunkCacheDim ) ];
				Chunk* chunkNegXPosZ = &gChunkCache[ ChunkCacheIndexFromChunkPos( x-1, z+1, gChunkCacheDim ) ];
				Chunk* chunkPosXNegZ = &gChunkCache[ ChunkCacheIndexFromChunkPos( x+1, z-1, gChunkCacheDim ) ];
				Chunk* chunkNegXNegZ = &gChunkCache[ ChunkCacheIndexFromChunkPos( x-1, z-1, gChunkCacheDim ) ];

				GenerateChunkMesh( chunkMesh, chunkNegXPosZ,	chunkPosZ,	chunkPosXPosZ,
											  chunkNegX,		chunk,		chunkPosX,
											  chunkNegXNegZ,	chunkNegZ,	chunkPosXNegZ );

				renderer.SubmitChunkMesh( MeshCacheIndexFromChunkPos( x, z, gChunkMeshCacheDim ), chunkMesh->vertices, chunkMesh->numVertices );
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

	gSunDirection.y = sin( sunAzimuth ) * cos( XM_PIDIV2 - sunElevation );
	gSunDirection.z = sin( sunAzimuth ) * sin( XM_PIDIV2 - sunElevation );
	gSunDirection.x = cos( sunAzimuth );

	float smRegionDim = (float)gChunkMeshCacheDim * CHUNK_WIDTH;

	ProfileStart( "ShadowmapPass" );
	XMMATRIX lightProj = XMMatrixOrthographicLH(	smRegionDim*0.5f,
													smRegionDim*0.5f,
													-smRegionDim*0.5f,
													smRegionDim*0.5f );

	XMVECTOR vSunDirection = -XMVector4Normalize( XMLoadFloat3( &gSunDirection ) );
	XMVECTOR lightUp = XMVector4Normalize( XMVectorSet( 0.0f, cos( sunElevation ), -sin( sunElevation ), 0.0f ) );
	XMVECTOR lightRight = XMVector4Normalize( XMVector3Cross( lightUp, vSunDirection ) );

	XMMATRIX worldToLight = XMMatrixTranspose( XMMATRIX(
		lightRight,
		lightUp,
		vSunDirection,
		XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
	XMMATRIX lightToWorld = XMMatrixTranspose( worldToLight );

	float texelsPerMeter = SM_RESOLUTION / smRegionDim;

	XMVECTOR vLightPos = vPos;// - (0.5f*SM_REGION_DIM) * vSunDirection;
	vLightPos = XMVector4Transform( vLightPos, worldToLight );
	vLightPos *= texelsPerMeter;
	vLightPos = XMVectorRound( vLightPos );
	vLightPos /= texelsPerMeter;
	vLightPos = XMVector4Transform( vLightPos, lightToWorld );

	XMMATRIX lightView =  XMMatrixLookToLH( vLightPos, vSunDirection, lightUp );
	XMMATRIX lightVP = XMMatrixTranspose( XMMatrixMultiply( lightView, lightProj ) );

	LightCB lightCBData;
	XMStoreFloat4x4( &lightCBData.vp, lightVP );

	renderer.SetChunkDrawingState();
	renderer.SetDepthBufferMode( DB_ENABLED );
	renderer.SetRasterizer( RS_SHADOWMAP );
	renderer.SetSampler( SAMPLER_POINT, ST_FRAGMENT, 1 );
	renderer.SetShader( 1 );
	renderer.RemoveTexture( ST_FRAGMENT, 1 );

	renderer.SetLightCBuffer( lightCBData );

	D3D11_VIEWPORT smViewport;
	smViewport.Width = SM_RESOLUTION;
	smViewport.Height = SM_RESOLUTION;
	smViewport.MinDepth = 0.0f;
	smViewport.MaxDepth = 1.0f;
	smViewport.TopLeftX = 0.0f;
	smViewport.TopLeftY = 0.0f;

	renderer.SetViewport( &smViewport );

	renderer.ClearTexture( &gShadowRT );
	renderer.ClearTexture( &gShadowDB );

	renderer.SetRenderTarget( &gShadowRT, &gShadowDB );
	ProfileStart( "ShadowmapDrawing" );
	for( int meshIndex = 0; meshIndex < gChunkMeshCacheDim * gChunkMeshCacheDim; meshIndex++ )
	{
		if( gChunkMeshCache[ meshIndex ].vertices )
		{
			renderer.DrawChunkMeshBuffer( gChunkMeshCache[ meshIndex ].chunkPos[0] * CHUNK_WIDTH,
											gChunkMeshCache[ meshIndex ].chunkPos[1] * CHUNK_WIDTH,
											meshIndex,
											gChunkMeshCache[ meshIndex ].numVertices );
			numDrawnVertices += gChunkMeshCache[ meshIndex ].numVertices;
		}
	}
	ProfileStop(); // ShadowmapDrawing
	ProfileStop(); // ShadowmapPass

#if 1
	// draw to frame buffer
	// Frame CB
	renderer.SetChunkDrawingState();

	FrameCB cbData;
	float screenAspect = (float)renderer.GetViewportWidth() / (float)renderer.GetViewportHeight();
	float verticalFOV = XMConvertToRadians( gPlayerHFOV / screenAspect );

	// frustum
	Frustum playerFrustum;
	float halfWidth = gPlayerNearPlane * tan( XMConvertToRadians( gPlayerHFOV )*0.5f );
	float halfHeight = gPlayerNearPlane * tan( verticalFOV*0.5f );

	XMFLOAT3 frustumA;
	XMFLOAT3 frustumB;
	XMFLOAT3 frustumC;
	XMFLOAT3 frustumD;
	XMFLOAT3 frustumE;
	XMFLOAT3 frustumF;
	XMFLOAT3 frustumG;
	XMFLOAT3 frustumH;

	XMVECTOR vLookUp = XMVector3Cross( vLook, vRight );

	XMStoreFloat3( &frustumA, vEyePos + vLook*gPlayerNearPlane - vRight*halfWidth - vLookUp*halfHeight );
	XMStoreFloat3( &frustumB, vEyePos + vLook*gPlayerNearPlane + vRight*halfWidth - vLookUp*halfHeight );
	XMStoreFloat3( &frustumC, vEyePos + vLook*gPlayerNearPlane + vRight*halfWidth + vLookUp*halfHeight );
	XMStoreFloat3( &frustumD, vEyePos + vLook*gPlayerNearPlane - vRight*halfWidth + vLookUp*halfHeight );

	halfWidth = gPlayerFarPlane * tan( XMConvertToRadians( gPlayerHFOV )*0.5f );
	halfHeight = gPlayerFarPlane * tan( verticalFOV*0.5f );

	XMStoreFloat3( &frustumE, vEyePos + vLook*gPlayerFarPlane - vRight*halfWidth - vLookUp*halfHeight );
	XMStoreFloat3( &frustumF, vEyePos + vLook*gPlayerFarPlane + vRight*halfWidth - vLookUp*halfHeight );
	XMStoreFloat3( &frustumG, vEyePos + vLook*gPlayerFarPlane + vRight*halfWidth + vLookUp*halfHeight );
	XMStoreFloat3( &frustumH, vEyePos + vLook*gPlayerFarPlane - vRight*halfWidth + vLookUp*halfHeight );

	playerFrustum = { frustumA, frustumB, frustumC, frustumD, frustumE, frustumF, frustumG, frustumH };

	playerProj = XMMatrixPerspectiveFovLH( verticalFOV, screenAspect, gPlayerNearPlane, gPlayerFarPlane );
	playerView =  XMMatrixLookToLH( vEyePos, vLook, vUp );
	XMMATRIX vp = XMMatrixTranspose( XMMatrixMultiply( playerView, playerProj ) );

	XMMATRIX debugTopView = XMMatrixLookToLH( vEyePos + vUp*100.0f, -vUp, vLook );
	// XMMATRIX debugTopProjection = XMMatrixPerspectiveFovLH( XM_PI / 4, screenAspect, 0.01f, 150.0f );
	XMMATRIX debugTopProjection = XMMatrixOrthographicLH( 100.0f * screenAspect, 100.0f, 1.0f, 150.0f );
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

	cbData.sunColor = { gSunColor.x * Saturate( gSunDirection.y ),
						gSunColor.x * Saturate( gSunDirection.y ),
						gSunColor.x * Saturate( gSunDirection.y ),
						1.0f };
	cbData.ambientColor = { gAmbientColor.x * Clamp( gSunDirection.y, 0.2f, 1.0f ),
							gAmbientColor.y * Clamp( gSunDirection.y, 0.2f, 1.0f ),
							gAmbientColor.z * Clamp( gSunDirection.y, 0.2f, 1.0f ),
							1.0f };

	cbData.sunDirection = XMFLOAT4( gSunDirection.x,
									gSunDirection.y,
									gSunDirection.z,
									0.0f );

	cbData.dayTimeNorm = t;

	cbData.lightVP = lightCBData.vp;

	renderer.SetFrameCBuffer( cbData );


	renderer.SetViewport( 0 );

	renderer.SetDepthBufferMode( DB_ENABLED );
	renderer.SetRasterizer( RS_DEFAULT );
	renderer.SetShader( 0 );
	renderer.SetRenderTarget( 0, 0 );
	renderer.SetTexture( gShadowRT, ST_FRAGMENT, 4 );

	int numChunksToDraw = 0;
	int numChunksDrawn = 0;
	Plane frustumPlanes[6];
	frustumPlanes[0] = Plane( playerFrustum.corners[ 0 ], playerFrustum.corners[ 3 ], playerFrustum.corners[ 4 ] );
	frustumPlanes[1] = Plane( playerFrustum.corners[ 0 ], playerFrustum.corners[ 4 ], playerFrustum.corners[ 1 ] );
	frustumPlanes[2] = Plane( playerFrustum.corners[ 1 ], playerFrustum.corners[ 5 ], playerFrustum.corners[ 2 ] );
	frustumPlanes[3] = Plane( playerFrustum.corners[ 2 ], playerFrustum.corners[ 6 ], playerFrustum.corners[ 3 ] );
	frustumPlanes[4] = Plane( playerFrustum.corners[ 0 ], playerFrustum.corners[ 1 ], playerFrustum.corners[ 3 ] );
	frustumPlanes[5] = Plane( playerFrustum.corners[ 5 ], playerFrustum.corners[ 4 ], playerFrustum.corners[ 6 ] );
	ProfileStart( "BeautyPass" );
	for( int meshIndex = 0; meshIndex < gChunkMeshCacheDim * gChunkMeshCacheDim; meshIndex++ )
	{
		if( gChunkMeshCache[ meshIndex ].vertices )
		{
			numChunksToDraw++;
			bool culled = false;

			AABB chunkBound;
			chunkBound.min = {
				float( gChunkMeshCache[ meshIndex ].chunkPos[0] * CHUNK_WIDTH ),
				0,
				float( gChunkMeshCache[ meshIndex ].chunkPos[1] * CHUNK_WIDTH ) };
			chunkBound.max = {
				chunkBound.min.x + CHUNK_WIDTH,
				CHUNK_HEIGHT,
				chunkBound.min.z + CHUNK_WIDTH };

			for( int planeIndex = 0; planeIndex < 6; planeIndex++ )
			{
				if( TestIntersection( frustumPlanes[ planeIndex ], chunkBound ) == OUTSIDE )
				{
					culled = true;
					break;
				}
			}

			ProfileStart( "BeatyDrawing" );
			if( !culled )
			{
				renderer.DrawChunkMeshBuffer( gChunkMeshCache[ meshIndex ].chunkPos[0] * CHUNK_WIDTH,
											gChunkMeshCache[ meshIndex ].chunkPos[1] * CHUNK_WIDTH,
											meshIndex,
											gChunkMeshCache[ meshIndex ].numVertices );
				numDrawnVertices += gChunkMeshCache[ meshIndex ].numVertices;
				numChunksDrawn++;
			}
			ProfileStop();

		}
	}
	ProfileStop();

#endif
	ProfileStop();

	if( blockPicked )
	{
		overlay.OulineBlock( 	pickedBlock.chunkX,
								pickedBlock.chunkZ,
								pickedBlock.x,
								pickedBlock.y,
								pickedBlock.z );
	}

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
		overlay.WriteLine( "Player pos: %5.2f %5.2f %5.2f", playerPos.x, playerPos.y, playerPos.z );
		overlay.WriteLine( "Chunk pos:  %5i ----- %5i", playerChunkPos.x, playerChunkPos.z );
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
		overlay.WriteLine( "Drawing %i / %i chunks (%i culled)", numChunksDrawn,
																 numChunksToDraw,
																 numChunksToDraw - numChunksDrawn );


		ProfileStop();

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

		// Draw frustum
		{
			overlay.DrawLine( playerFrustum.corners[0], playerFrustum.corners[1], { 0.0f, 0.5f, 1.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[1], playerFrustum.corners[2], { 0.0f, 0.5f, 1.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[2], playerFrustum.corners[3], { 0.0f, 0.5f, 1.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[3], playerFrustum.corners[0], { 0.0f, 0.5f, 1.0f, 1.0f } );

			overlay.DrawLine( playerFrustum.corners[4], playerFrustum.corners[5], { 0.0f, 0.5f, 1.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[5], playerFrustum.corners[6], { 0.0f, 0.5f, 1.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[6], playerFrustum.corners[7], { 0.0f, 0.5f, 1.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[7], playerFrustum.corners[4], { 0.0f, 0.5f, 1.0f, 1.0f } );

			overlay.DrawLine( playerFrustum.corners[0], playerFrustum.corners[4], { 1.0f, 0.5f, 0.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[1], playerFrustum.corners[5], { 1.0f, 0.5f, 0.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[2], playerFrustum.corners[6], { 1.0f, 0.5f, 0.0f, 1.0f } );
			overlay.DrawLine( playerFrustum.corners[3], playerFrustum.corners[7], { 1.0f, 0.5f, 0.0f, 1.0f } );
		} // End Draw frustum

		if( blockPicked )
		{
			overlay.DrawPoint( intersection.point, { 0.0f, 1.0f, 1.0f, 1.0f } );
		}

		XMFLOAT3 lightPos;
		XMStoreFloat3( &lightPos, vLightPos );
		overlay.DrawPoint( lightPos, { 1.0f, 0.0f, 1.0f, 1.0f } );
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
