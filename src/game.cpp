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
}

Game::~Game()
{
	if( world_ ) {
		delete world_;
	}
}

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

	world_ = new World();
	GenerateWorld( world_ );

	return true;
}

XMFLOAT3 playerPos = 	{ 0.0f, 14.0f, 0.0f };
XMFLOAT3 playerDir = 	{ 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerLook = 	{ 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerRight = 	{ 1.0f,  0.0f, 0.0f };
XMFLOAT3 playerUp = 	{ 0.0f,  1.0f, 0.0f };

int playerChunkX = 0;
int playerChunkZ = 0;

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

	const int chunksToDraw = 4;

	// draw 7*7 chunks around player
	for( int z = playerChunkZ - chunksToDraw; z <= playerChunkZ + chunksToDraw; z++ )
	{
		for( int x = playerChunkX - chunksToDraw; x <= playerChunkX + chunksToDraw; x++ )
		{
			//if( !ChunkInCache( x, z ) )
			{
				for( int blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
				{
					for( int blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
					{
						int height = 0;
						Chunk* chunk = &world_->chunks[x+16][z+16];

						Chunk* chunkPosX = &world_->chunks[x+16+1][z+16];
						Chunk* chunkNegX = &world_->chunks[x+16-1][z+16];
						Chunk* chunkPosZ = &world_->chunks[x+16][z+16+1];
						Chunk* chunkNegZ = &world_->chunks[x+16][z+16-1];

						while( chunk->blocks[blockX][height][blockZ] != BT_AIR )
						{
							if( blockX == CHUNK_WIDTH - 1 || chunk->blocks[blockX + 1][height][blockZ] == BT_AIR )
							{
								renderer.SubmitFace( XMFLOAT3( x * CHUNK_WIDTH + blockX, height, z * CHUNK_WIDTH + blockZ ), FACE_POS_X );
								numDrawnVertices += VERTS_PER_BLOCK;
							}
							if( blockX == 0 || chunk->blocks[blockX - 1][height][blockZ] == BT_AIR )
							{
								renderer.SubmitFace( XMFLOAT3( x * CHUNK_WIDTH + blockX, height, z * CHUNK_WIDTH + blockZ ), FACE_NEG_X );
								numDrawnVertices += VERTS_PER_BLOCK;
							}
							if( blockZ == CHUNK_WIDTH - 1 || chunk->blocks[blockX][height][blockZ + 1] == BT_AIR )
							{
								renderer.SubmitFace( XMFLOAT3( x * CHUNK_WIDTH + blockX, height, z * CHUNK_WIDTH + blockZ ), FACE_POS_Z );
								numDrawnVertices += VERTS_PER_BLOCK;
							}
							if( blockZ == 0 || chunk->blocks[blockX][height][blockZ - 1] == BT_AIR )
							{
								renderer.SubmitFace( XMFLOAT3( x * CHUNK_WIDTH + blockX, height, z * CHUNK_WIDTH + blockZ ), FACE_NEG_Z );
								numDrawnVertices += VERTS_PER_BLOCK;
							}
							if( height == CHUNK_HEIGHT || chunk->blocks[blockX][height + 1][blockZ] == BT_AIR )
							{
								renderer.SubmitFace( XMFLOAT3( x * CHUNK_WIDTH + blockX, height, z * CHUNK_WIDTH + blockZ ), FACE_POS_Y );
								numDrawnVertices += VERTS_PER_BLOCK;
							}

							height++;
						}
					}
				}
			}
		}
	}

	// draw remeining verts
	renderer.Flush();
//	if( batchVertexCount ) {
//		renderer.Draw( batchVertexCount );
//		numDrawnBatches++;
//	}

	ProfileStart( "Overlay" );

	float delta = sum / UPDATE_DELTA_FRAMES;
	int fps = 1000 / delta;

	overlay.Reset();
	overlay.WriteLine( "Frame time: %5.2f (%i fps)", sum / UPDATE_DELTA_FRAMES, fps);
	overlay.WriteLine( "Chunk buffer size: %i KB", sizeof( VertexPosNormalTexcoord ) * MAX_VERTS_PER_BATCH / 1024 );
	overlay.WriteLine( "Batches rendered: %i", renderer.numBatches_ );
	overlay.WriteLine( "Vertices rendered: %i", numDrawnVertices );
	overlay.WriteLine( "Mouse offset: %+03i - %+03i", input.mouse.x, input.mouse.y );
	overlay.WriteLine( "" );
	overlay.WriteLine( "Player pos: %5.2f %5.2f %5.2f", playerPos.x, playerPos.y, playerPos.z );
	overlay.WriteLine( "Chunk pos:  %5i ----- %5i", playerChunkX, playerChunkZ );

	ProfileStop();

	renderer.End();

	input.mouse.x = 0;
	input.mouse.y = 0;
}

}