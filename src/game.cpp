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

void Game::DoFrame()
{
	renderer.Begin();

	int batchVertexCount = 0;
	int numBatches = 0;

	const int chunksToDraw = 8;

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
							renderer.SubmitBlock( XMFLOAT3( CHUNK_WIDTH * (x - chunksToDraw / 2) + chunkX, chunkY, CHUNK_WIDTH * (z - chunksToDraw / 2) + chunkZ ) );
							// renderer.SubmitBlock( XMFLOAT3( chunkX, chunkY, chunkZ ) );
							batchVertexCount += VERTS_PER_BLOCK;

							if( batchVertexCount == MAX_VERTS_PER_BATCH ) {
								renderer.Draw( batchVertexCount );
								batchVertexCount = 0;
								numBatches++;
							}

						}
					}
				}
			}
		}
	}

	// draw remeining verts
	if( batchVertexCount ) {
		renderer.Draw( batchVertexCount );
		numBatches++;
	}

	overlay.Reset();
	overlay.Write( "Batches rendered: " );
	overlay.WriteLine( std::to_string( numBatches ).c_str() );

	renderer.End();
}

}