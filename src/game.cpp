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
	ProfileNewFrame( dt );
	renderer.Begin();

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


	int batchVertexCount = 0;
	int numDrawnBatches = 0;
	int numDrawnVertices = 0;

	const int chunksToDraw = 8;

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

	if( input.key[ 'W' ] ) {
		overlay.WriteLine( "W key pressed" );
	}
	ProfileStop();

	renderer.End();
}

}