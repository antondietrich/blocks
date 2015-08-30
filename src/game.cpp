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

	for( int i = 0; i < NUM_VKEYS; i++ )
	{
		input.key[i] = false;
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

XMFLOAT3 playerPos 		= { 0.0f, 20.0f, 0.0f };
XMFLOAT3 playerDir 		= { 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerLook 	= { 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerRight 	= { 1.0f,  0.0f, 0.0f };
XMFLOAT3 playerUp 		= { 0.0f,  1.0f, 0.0f };
XMFLOAT3 playerSpeed 	= { 0.0f,  0.0f, 0.0f };
float playerMass = 75.0f;
float playerHeight = 1.8f;

int playerChunkX = 0;
int playerChunkZ = 0;

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

	// player movement
	float dTSec = dt / 1000.0f;
	XMVECTOR vPos, vDir, vLook, vRight, vUp, vSpeed, force, acceleration, drag, gravity;
	vPos 	= XMLoadFloat3( &playerPos );
	vDir 	= XMLoadFloat3( &playerDir );
	vLook 	= XMLoadFloat3( &playerLook );
	vRight 	= XMLoadFloat3( &playerRight );
	vUp 	= XMLoadFloat3( &playerUp );
	vSpeed 	= XMLoadFloat3( &playerSpeed );

	force = XMVectorZero();
	acceleration = XMVectorZero();
	gravity = XMVectorSet( 0.0f, -9.8f, 0.0f, 0.0f ) * playerMass;

	if( input.key[ 'W' ] ) {
		force += vDir;
	}
	if( input.key[ 'S' ] ) {
		force -= vDir;
	}
	if( input.key[ 'D' ] ) {
		force += vRight;
	}
	if( input.key[ 'A' ] ) {
		force -= vRight;
	}
	if( input.key[ VK_SPACE ] ) {
		force += vUp;
	}
	force = XMVector4Normalize( force ) * 1500.0f;
	force += gravity;

	drag = 0.5f * vSpeed;
	acceleration = force / playerMass - drag;

	vPos = vPos + vSpeed * dTSec + ( acceleration * dTSec * dTSec ) / 2.0f;
	vSpeed = vSpeed + acceleration * dTSec;

	// TEMP: ground the player
	if( playerPos.x >= 0 ) {
		playerChunkX = (int)playerPos.x / CHUNK_WIDTH;
	}
	else {
		playerChunkX = (int)playerPos.x / CHUNK_WIDTH - 1;
	}

	if( playerPos.z >= 0 ) {
		playerChunkZ = (int)playerPos.z / CHUNK_WIDTH;
	}
	else {
		playerChunkZ = (int)playerPos.z / CHUNK_WIDTH - 1;
	}

	int groundHeight = 20;

	int playerBlockX = (int)floor( playerPos.x - playerChunkX * CHUNK_WIDTH );
	int playerBlockZ = (int)floor( playerPos.z - playerChunkZ * CHUNK_WIDTH );

	int index = ChunkCacheIndexFromChunkPos( playerChunkX, playerChunkZ );
	Chunk *chunk = &world_->chunks[ index ];

	for( int i = 0; i < CHUNK_HEIGHT; i++ )
	{
		if( chunk->blocks[playerBlockX][i][playerBlockZ] == BT_AIR )
		{
			groundHeight = i;
			break;
		}
	}

	vPos = XMVectorSetY( vPos, (float)groundHeight );

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
	int chunksGenerated = 0;
	//BlockVertex *vertexBuffer = new BlockVertex[ MAX_VERTS_PER_CHUNK_MESH ];

	// const int CHUNKS_TO_DRAW = 4;
	// TODO: key state (down, up, press, release)
	if( input.key[ VK_F1 ] ) {
		gDrawOverlay = !gDrawOverlay;
	}

	assert( CHUNKS_TO_DRAW <= CHUNK_CACHE_DIM );

	for( int z = playerChunkZ - CHUNK_CACHE_HALF_DIM; z <= playerChunkZ + CHUNK_CACHE_HALF_DIM; z++ )
	{
		for( int x = playerChunkX - CHUNK_CACHE_HALF_DIM; x <= playerChunkX + CHUNK_CACHE_HALF_DIM; x++ )
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

	for( int z = playerChunkZ - CHUNKS_TO_DRAW; z <= playerChunkZ + CHUNKS_TO_DRAW; z++ )
	{
		for( int x = playerChunkX - CHUNKS_TO_DRAW; x <= playerChunkX + CHUNKS_TO_DRAW; x++ )
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
		overlay.WriteLine( "Chunk pos:  %5i ----- %5i", playerChunkX, playerChunkZ );
		overlay.WriteLine( "Speed:  %5.2f", XMVectorGetX( XMVector4Length( vSpeed ) ) );

		ProfileStop();
	}

	renderer.End();

	input.mouse.x = 0;
	input.mouse.y = 0;
}

}