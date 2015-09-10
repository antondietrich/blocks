#include "game.h"

namespace Blocks
{

using namespace DirectX;

bool Game::isInstantiated_ = false;

Game::Game()
:
renderer()
{
	assert( !isInstantiated_ );
	isInstantiated_ = true;

	world_ = 0;
	//chunkVertexBuffer_ = 0;
}

Game::~Game()
{
	if( world_ ) {
		delete world_;
	}
//	if( chunkVertexBuffer_ ) {
//		delete[] chunkVertexBuffer_;
//	}
}

ChunkMesh chunkMeshCache[ MESH_CACHE_DIM * MESH_CACHE_DIM ];

bool Game::Start( HWND wnd )
{
	if( !renderer.Start( wnd ) )
	{
		return false;
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

	InitWorldGen();
	world_ = new World();
	GenerateWorld( world_ );

	for( int i = 0; i < MESH_CACHE_DIM * MESH_CACHE_DIM; i++ ) {
		chunkMeshCache[i].vertices = 0;
		chunkMeshCache[i].size = 0;
		chunkMeshCache[i].chunkPos[0] = 0;
		chunkMeshCache[i].chunkPos[1] = 0;
	}

	

	return true;
}

XMFLOAT3 playerPos 		= { 0.0f, 120.0f, 0.0f };
XMFLOAT3 playerDir 		= { 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerLook 	= { 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerRight 	= { 1.0f,  0.0f, 0.0f };
XMFLOAT3 playerUp 		= { 0.0f,  1.0f, 0.0f };
XMFLOAT3 playerSpeed 	= { 0.0f,  0.0f, 0.0f };
XMFLOAT3 gravity 		= { 0.0f,  0.0f, 0.0f };

float playerMass = 75.0f;
float playerHeight = 1.8f;
bool playerAirborne = true;

bool gDrawOverlay = true;

void Game::DoFrame( float dt )
{
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
	for( int z = playerChunkPos.z - CHUNK_CACHE_HALF_DIM; z <= playerChunkPos.z + CHUNK_CACHE_HALF_DIM; z++ )
	{
		for( int x = playerChunkPos.x - CHUNK_CACHE_HALF_DIM; x <= playerChunkPos.x + CHUNK_CACHE_HALF_DIM; x++ )
		{
			int index = ChunkCacheIndexFromChunkPos( x, z );
			Chunk *chunk = &world_->chunks[ index ];

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
	if( input.key[ KEY::LCTRL ].Pressed ) {
		if( fabs( gravity.y - 0.0f ) < 0.000001f )
		{
			gravity.y = -9.8f;
		}
		else
		{
			gravity.y = 0.0f;
		}
	}
	
	force = XMVector4Normalize( force ) * 1500.0f;
	
	if( input.key[ KEY::SPACE ].Down ) {
		if( !playerAirborne ) {
			force += vUp * 2400.0 * playerMass * 2.35 / dt;
			playerAirborne = true;
		}
	}
	
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

	BLOCK_TYPE targetBlockTypeX = GetBlockType( world_->chunks[ ChunkCacheIndexFromChunkPos(targetChunkPosX.x, targetChunkPosX.z) ], targetBlockPosX );
	if( targetBlockTypeX != BT_AIR )
	{
		playerDeltaX = 0.0f;
		vSpeed = XMVectorSetX( vSpeed, 0.0f );
	}

	BLOCK_TYPE targetBlockTypeY = GetBlockType( world_->chunks[ ChunkCacheIndexFromChunkPos(targetChunkPosY.x, targetChunkPosY.z) ], targetBlockPosY );
	if( targetBlockTypeY != BT_AIR && playerSpeed.y < 0.0f )
	{
		playerDeltaY = 0.0f;
		playerAirborne = false;
		vSpeed = XMVectorSetY( vSpeed, 0.0f );
		acceleration = XMVectorSetY( acceleration, -9.8f );
	}

	BLOCK_TYPE targetBlockTypeZ = GetBlockType( world_->chunks[ ChunkCacheIndexFromChunkPos(targetChunkPosZ.x, targetChunkPosZ.z) ], targetBlockPosZ );
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

	renderer.SetView( playerEyePos, playerLook, playerUp );

	// world rendering
//	int batchVertexCount = 0;
//	int numDrawnBatches = 0;
	int numDrawnVertices = 0;
	int chunkMeshesRebuilt = 0;

	// TODO: key state (down, up, press, release)
	if( input.key[ KEY::F1 ].Pressed ) {
		gDrawOverlay = !gDrawOverlay;
	}


	// render chunks around player
	assert( CHUNKS_TO_DRAW <= CHUNK_CACHE_DIM );

	for( int z = playerChunkPos.z - CHUNKS_TO_DRAW; z <= playerChunkPos.z + CHUNKS_TO_DRAW; z++ )
	{
		for( int x = playerChunkPos.x - CHUNKS_TO_DRAW; x <= playerChunkPos.x + CHUNKS_TO_DRAW; x++ )
		{
			ChunkMesh *chunkMesh = &chunkMeshCache[ MeshCacheIndexFromChunkPos( x, z ) ];
			if( chunkMesh->vertices && chunkMesh->chunkPos[0] == x && chunkMesh->chunkPos[1] == z )
			{
				// use chunk mesh
			}
			else
			{
				// rebuild chunk mesh
				chunkMeshesRebuilt++;

				chunkMesh->chunkPos[0] = x;
				chunkMesh->chunkPos[1] = z;

				if( chunkMesh->vertices ) {
					delete[] chunkMesh->vertices;
					chunkMesh->vertices = 0;
				}

				// TODO: any reason to break here?
				if( chunkMeshesRebuilt > 1 ) {
					break;
				}
				
				Chunk* chunk = &world_->chunks[ ChunkCacheIndexFromChunkPos( x, z ) ];

				Chunk* chunkPosX = &world_->chunks[ ChunkCacheIndexFromChunkPos( x+1, z ) ];
				Chunk* chunkNegX = &world_->chunks[ ChunkCacheIndexFromChunkPos( x-1, z ) ];
				Chunk* chunkPosZ = &world_->chunks[ ChunkCacheIndexFromChunkPos( x, z+1 ) ];
				Chunk* chunkNegZ = &world_->chunks[ ChunkCacheIndexFromChunkPos( x, z-1 ) ];

				Chunk* chunkPosXPosZ = &world_->chunks[ ChunkCacheIndexFromChunkPos( x+1, z+1 ) ];
				Chunk* chunkNegXPosZ = &world_->chunks[ ChunkCacheIndexFromChunkPos( x-1, z+1 ) ];
				Chunk* chunkPosXNegZ = &world_->chunks[ ChunkCacheIndexFromChunkPos( x+1, z-1 ) ];
				Chunk* chunkNegXNegZ = &world_->chunks[ ChunkCacheIndexFromChunkPos( x-1, z-1 ) ];

				GenerateChunkMesh( chunkMesh, chunkNegXPosZ,	chunkPosZ,	chunkPosXPosZ,
											  chunkNegX,		chunk,		chunkPosX,
											  chunkNegXNegZ,	chunkNegZ,	chunkPosXNegZ );
				
			} // else

		} // for chunkX
		if( chunkMeshesRebuilt > 1 ) {
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
	for( int meshIndex = 0; meshIndex < MESH_CACHE_DIM * MESH_CACHE_DIM; meshIndex++ )
	{
		if( chunkMeshCache[ meshIndex ].vertices )
		{
			renderer.DrawChunk( chunkMeshCache[ meshIndex ].chunkPos[0] * CHUNK_WIDTH,
								chunkMeshCache[ meshIndex ].chunkPos[1] * CHUNK_WIDTH,
								chunkMeshCache[ meshIndex ].vertices,
								chunkMeshCache[ meshIndex ].size );
			numDrawnVertices += chunkMeshCache[ meshIndex ].size;
		}
	}
	ProfileStop();

	if( gDrawOverlay )
	{
		ProfileStart( "Overlay" );

		float delta = sum / UPDATE_DELTA_FRAMES;
		int fps = (int)(1000 / delta);

		overlay.Reset();
		overlay.WriteLine( "Frame time: %5.2f (%i fps)", sum / UPDATE_DELTA_FRAMES, fps);
		overlay.WriteLine( "Chunk buffer size: %i KB", sizeof( BlockVertex ) * MAX_VERTS_PER_BATCH / 1024 );
//		overlay.WriteLine( "Batches rendered: %i", renderer.numBatches_ );
//		overlay.WriteLine( "Vertices rendered: %i", numDrawnVertices );
		overlay.WriteLine( "Chunks generated: %i", chunksGenerated );
		overlay.WriteLine( "Chunk meshes rebuild: %i", chunkMeshesRebuilt );
//		overlay.WriteLine( "Mouse offset: %+03i - %+03i", input.mouse.x, input.mouse.y );
		overlay.WriteLine( "" );
		overlay.WriteLine( "Player pos: %5.2f %5.2f %5.2f", playerPos.x, playerPos.y, playerPos.z );
		overlay.WriteLine( "Chunk pos:  %5i ----- %5i", playerChunkPos.x, playerChunkPos.z );
		overlay.WriteLine( "Speed:  %5.2f", XMVectorGetX( XMVector4Length( vSpeed ) ) );
		overlay.Write( "Keys pressed: " );

		for( int i = 0; i < KEY::COUNT; i++ )
		{
			if( input.key[i].Down )
			{
				overlay.Write( "%s, ", GetKeyName( (KEY)i ) );
			}
		}
		overlay.Write( "" );

		ProfileStop();
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