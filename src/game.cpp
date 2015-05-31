#include "game.h"

namespace Blocks
{

bool Game::isInstantiated_ = false;

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
	if( !renderer.Start( wnd ) )
	{
		return false;
	}

	if( !overlay.Start( &renderer ) )
	{
		return false;
	}


	return true;
}

void Game::DoFrame()
{
	renderer.Begin();
	//renderer.Present();
	overlay.Reset();
	overlay.WriteLine( "Hello, world!" );
	overlay.Write( "Second line, woah!" );
	overlay.Write( "And a very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, long line!!" );
	overlay.WriteLine( "Append this." );

	renderer.End();
}

}