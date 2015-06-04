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

//	world_ = new World();
//	GenerateWorld( world_ );

	return true;
}

void Game::DoFrame( float dt )
{
	Profile::NewFrame( dt );
	renderer.Begin();

	static int deltaFramesElapsed;
	static float capturedDelta;
	static char deltaStr[ 8 ];
	
	deltaFramesElapsed++;
	if( deltaFramesElapsed % UPDATE_DELTA_FRAMES == 0 ) {
		capturedDelta = dt;
	}

	sprintf( deltaStr, "%5.2f", capturedDelta );


	int batchVertexCount = 0;
	int numDrawnBatches = 0;
	int numDrawnVertices = 0;

	const int chunksToDraw = 8;

	Profile::Start( "Render chunks" );
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
							Profile::Start( "Renderer.submitBlock" );
							renderer.SubmitBlock( XMFLOAT3( CHUNK_WIDTH * (x - chunksToDraw / 2) + chunkX, chunkY, CHUNK_WIDTH * (z - chunksToDraw / 2) + chunkZ ) );
							Profile::Stop();
							// renderer.SubmitBlock( XMFLOAT3( chunkX, chunkY, chunkZ ) );
							batchVertexCount += VERTS_PER_BLOCK;
							numDrawnVertices += VERTS_PER_BLOCK;

							if( batchVertexCount == MAX_VERTS_PER_BATCH ) {
								renderer.Draw( batchVertexCount );
								batchVertexCount = 0;
								numDrawnBatches++;
							}

						}
					}
				}
			}
		}
	}
	Profile::Stop();

	// draw remeining verts
	if( batchVertexCount ) {
		renderer.Draw( batchVertexCount );
		numDrawnBatches++;
	}

	overlay.Reset();
	overlay.Write( "Frame time: " );
	overlay.WriteLine( deltaStr );
	overlay.Write( "Batches rendered: " );
	overlay.WriteLine( std::to_string( numDrawnBatches ).c_str() );
	overlay.Write( "Vertices rendered: " );
	overlay.WriteLine( std::to_string( numDrawnVertices ).c_str() );

	if( input.key[ 'W' ] ) {
		overlay.WriteLine( "W key pressed" );
	}

	renderer.End();
}

}