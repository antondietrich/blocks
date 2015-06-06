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

//	world_ = new World();
//	GenerateWorld( world_ );

	return true;
}

XMFLOAT3 playerPos = 	{ 0.0f, 14.0f, 0.0f };
XMFLOAT3 playerDir = 	{ 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerLook = 	{ 0.0f,  0.0f, 1.0f };
XMFLOAT3 playerRight = 	{ 1.0f,  0.0f, 0.0f };
XMFLOAT3 playerUp = 	{ 0.0f,  1.0f, 0.0f };

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

	// world rendering
	int batchVertexCount = 0;
	int numDrawnBatches = 0;
	int numDrawnVertices = 0;

	const int chunksToDraw = 4;

	ProfileStart( "Render chunks" );
	for( int z = 0; z < chunksToDraw; z++ )
	{
		for( int x = 0; x < chunksToDraw; x++ )
		{
			for( int chunkY = 0; chunkY < CHUNK_HEIGHT; chunkY++ )
			{
				for( int chunkZ = 0; chunkZ < CHUNK_WIDTH; chunkZ++ )
				{
					for( int chunkX = 0; chunkX < CHUNK_WIDTH; chunkX++ )
					{
						if( chunkY == 12 ) {
							ProfileStart( "Renderer.SubmitBlock" );
							renderer.SubmitBlock( XMFLOAT3( CHUNK_WIDTH * (x - chunksToDraw / 2) + chunkX, chunkY, CHUNK_WIDTH * (z - chunksToDraw / 2) + chunkZ ) );
							ProfileStop();
							// renderer.SubmitBlock( XMFLOAT3( chunkX, chunkY, chunkZ ) );
							batchVertexCount += VERTS_PER_BLOCK;
							numDrawnVertices += VERTS_PER_BLOCK;

							if( batchVertexCount == MAX_VERTS_PER_BATCH ) {
								ProfileStart( "Renderer.Draw" );
								renderer.Draw( batchVertexCount );
								ProfileStop();
								batchVertexCount = 0;
								numDrawnBatches++;
							}

						}
					}
				}
			}
		}
	}
	ProfileStop();

	// draw remeining verts
	if( batchVertexCount ) {
		renderer.Draw( batchVertexCount );
		numDrawnBatches++;
	}

	ProfileStart( "Overlay" );
	overlay.Reset();
	overlay.Write( "Frame time: " );
	overlay.WriteLine( deltaStr );
	overlay.Write( "Batches rendered: " );
	overlay.WriteLine( std::to_string( numDrawnBatches ).c_str() );
	overlay.Write( "Vertices rendered: " );
	overlay.WriteLine( std::to_string( numDrawnVertices ).c_str() );

	overlay.Write( "Mouse offset: " );
	overlay.Write( std::to_string( input.mouse.x ).c_str() );
	overlay.Write( ":" );
	overlay.WriteLine( std::to_string( input.mouse.y ).c_str() );
	ProfileStop();

	renderer.End();

	input.mouse.x = 0;
	input.mouse.y = 0;
}

}