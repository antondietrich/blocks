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

ChunkMesh chunkMeshCache[ VISIBLE_CHUNKS_RADIUS * VISIBLE_CHUNKS_RADIUS ];

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

	for( int i = 0; i < VISIBLE_CHUNKS_RADIUS * VISIBLE_CHUNKS_RADIUS; i++ ) {
		chunkMeshCache[i].vertices = 0;
		chunkMeshCache[i].size = 0;
		chunkMeshCache[i].chunkPos[0] = 0;
		chunkMeshCache[i].chunkPos[1] = 0;
	}

	

	return true;
}

XMFLOAT3 playerPos = 	{ 0.0f, 50.0f, 0.0f };
XMFLOAT3 playerDir = 	{ 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerLook = 	{ 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerRight = 	{ 1.0f,  0.0f, 0.0f };
XMFLOAT3 playerUp = 	{ 0.0f,  1.0f, 0.0f };

int playerChunkX = 0;
int playerChunkZ = 0;

bool gDrawOverlay = false;

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
	XMVECTOR vPos, vDir, vLook, vRight, vUp;
	vPos = XMLoadFloat3( &playerPos );
	vDir = XMLoadFloat3( &playerDir );
	vLook = XMLoadFloat3( &playerLook );
	vRight = XMLoadFloat3( &playerRight );
	vUp = XMLoadFloat3( &playerUp );

	if( input.key[ 'W' ] ) {
		vPos += vDir*0.1f;
	}
	if( input.key[ 'S' ] ) {
		vPos -= vDir*0.1f;
	}
	if( input.key[ 'D' ] ) {
		vPos += vRight*0.1f;
	}
	if( input.key[ 'A' ] ) {
		vPos -= vRight*0.1f;
	}
	if( input.key[ VK_SPACE ] ) {
		vPos += vUp*0.1f;
	}
	if( input.key[ VK_CONTROL ] ) {
		vPos -= vUp*0.1f;
	}
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

	renderer.SetView( playerPos, playerLook, playerUp );

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

	// world rendering
//	int batchVertexCount = 0;
//	int numDrawnBatches = 0;
	int numDrawnVertices = 0;
	int chunkMeshesRebuilt = 0;
	//BlockVertex *vertexBuffer = new BlockVertex[ MAX_VERTS_PER_CHUNK_MESH ];

	// const int CHUNKS_TO_DRAW = 4;

	if( input.key[ VK_F1 ] ) {
		gDrawOverlay = !gDrawOverlay;
	}

	for( int z = playerChunkZ - CHUNKS_TO_DRAW; z <= playerChunkZ + CHUNKS_TO_DRAW; z++ )
	{
		for( int x = playerChunkX - CHUNKS_TO_DRAW; x <= playerChunkX + CHUNKS_TO_DRAW; x++ )
		{
			// always positive
			uint storageX = x + INT_MAX;
			uint storageZ = z + INT_MAX;
			ChunkMesh *chunkMesh = &chunkMeshCache[ MeshCacheIndexFromChunkPos( storageX, storageZ ) ];
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

				if( chunkMeshesRebuilt > 1 ) {
					break;
				}
				
				Chunk* chunk = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE][z+HALF_CHUNKS_TO_GENERATE];

				Chunk* chunkPosX = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE+1][z+HALF_CHUNKS_TO_GENERATE];
				Chunk* chunkNegX = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE-1][z+HALF_CHUNKS_TO_GENERATE];
				Chunk* chunkPosZ = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE][z+HALF_CHUNKS_TO_GENERATE+1];
				Chunk* chunkNegZ = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE][z+HALF_CHUNKS_TO_GENERATE-1];

				Chunk* chunkPosXPosZ = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE+1][z+HALF_CHUNKS_TO_GENERATE+1];
				Chunk* chunkNegXPosZ = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE-1][z+HALF_CHUNKS_TO_GENERATE+1];
				Chunk* chunkPosXNegZ = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE+1][z+HALF_CHUNKS_TO_GENERATE-1];
				Chunk* chunkNegXNegZ = &world_->chunks[x+HALF_CHUNKS_TO_GENERATE-1][z+HALF_CHUNKS_TO_GENERATE-1];

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

//	if( batchVertexCount ) {
//		renderer.Draw( batchVertexCount );
//		numDrawnBatches++;
//	}

	ProfileStart( "Render" );
	for( int meshIndex = 0; meshIndex < VISIBLE_CHUNKS_RADIUS * VISIBLE_CHUNKS_RADIUS; meshIndex++ )
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
		overlay.WriteLine( "Batches rendered: %i", renderer.numBatches_ );
		overlay.WriteLine( "Vertices rendered: %i", numDrawnVertices );
		overlay.WriteLine( "Chunk meshes rebuild: %i", chunkMeshesRebuilt );
		overlay.WriteLine( "Mouse offset: %+03i - %+03i", input.mouse.x, input.mouse.y );
		overlay.WriteLine( "" );
		overlay.WriteLine( "Player pos: %5.2f %5.2f %5.2f", playerPos.x, playerPos.y, playerPos.z );
		overlay.WriteLine( "Chunk pos:  %5i ----- %5i", playerChunkX, playerChunkZ );

		ProfileStop();
	}

	renderer.End();

	input.mouse.x = 0;
	input.mouse.y = 0;
}

}