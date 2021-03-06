#include <Windows.h>

#define __BLOCKS_INPUT_IMPL__
#include "lib/win32_file.h"
#include "lib/tokenstream.h"
#include "input.h"
#include "types.h"
#include "game.h"
#include "config.h"
#include "utils.h"

#include <map>

using namespace Blocks;

ConfigType Blocks::Config;

RECT gClipRect;
RECT gOldClipRect;
bool gWindowMinimized = false;

LRESULT CALLBACK WndProc( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam );
bool LoadConfig();
bool RegisterRawInputDevices( HWND hwnd );
void TranslateUserInput( UserInput &userInput, LPARAM lparam );
void GetScreenSpaceClientRect( HWND hwnd, RECT * rect );

int __stdcall wWinMain( HINSTANCE thisInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow )
{
	UNREFERENCED_PARAMETER( prevInstance );
	UNREFERENCED_PARAMETER( cmdLine );

	/* Load config */
	if( !LoadConfig() )
	{
		OutputDebugStringA( "Failed to load config!" );
		return 1;
	}

	/* Create a window */
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof( WNDCLASSEX );
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WndProc;
	windowClass.hInstance = thisInstance;
	windowClass.hIcon = LoadIcon(thisInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	windowClass.hCursor = LoadCursor( NULL, IDC_ARROW );
	windowClass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = "BlocksWindowClass";

	if( !RegisterClassEx( &windowClass ) ) {
		OutputDebugStringA( "Failed to register window class!" );
		return 1;
	}

	RECT clientRect, desktopRect;
	HWND desktop = GetDesktopWindow();
	GetWindowRect( desktop, &desktopRect );
	if( Config.fullscreen || Config.screenWidth == 0 || Config.screenHeight == 0 )
	{
		clientRect = desktopRect;
	}
	else
	{
		clientRect.left = 10;
		clientRect.top = 30;
		clientRect.right = Config.screenWidth;
		clientRect.bottom = Config.screenHeight;
	}
	AdjustWindowRect( &clientRect, WS_OVERLAPPEDWINDOW, FALSE );

	HWND window = CreateWindow( "BlocksWindowClass", "Blocks", WS_OVERLAPPEDWINDOW,
								clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
								NULL, NULL, thisInstance, NULL );
	if( !window ) {
		OutputDebugStringA( "Failed to create window!" );
		return 1;
	}

	ShowWindow( window, cmdShow );
	UpdateWindow( window );

	// clip cursor
	GetClipCursor( &gOldClipRect );
	GetScreenSpaceClientRect( window, &gClipRect );
	ClipCursor( &gClipRect );

	if( !RegisterRawInputDevices( window ) ) {
		OutputDebugStringA( "Failed to register raw input devices!" );
		return 1;
	}

	HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to show main window!" );
		return 1;
	}

	Game game = Game();
	if( !game.Start( window ) )
	{
		OutputDebugStringA( "Failed to start the game" );
		return 1;
	}

	/* pass a game pointer to the window for use in WndProc */
	SetWindowLongPtr( window, GWLP_USERDATA, (LONG_PTR)&game );

	/* Main loop */

	LONGLONG startTime = 0;
	LONGLONG endTime = 0;
	float delta = 0;

	MSG msg = { 0 };
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			// NOTE: using previous frame delta
			delta = (float)( Time::ToMiliseconds( endTime - startTime ) );
			startTime = Time::Now();
			game.DoFrame( delta );
			endTime = Time::Now();
		}
	}

	ProfileReport();

	return (int) msg.wParam;
}

// TODO: need better way to communicate with game
// TODO: handle maximize command properly
LRESULT CALLBACK WndProc( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT paintStruct;
	HDC hDC;

	Game *game = 0;

	switch( message )
	{
		case WM_INPUT:
			game = (Game*)GetWindowLong( windowHandle, GWLP_USERDATA );
			TranslateUserInput( game->input, lParam );
			break;
		case WM_KEYDOWN:
			switch( wParam )
			{
				case VK_ESCAPE:
					PostQuitMessage( 0 );
					break;
			}
			break;
		case WM_SYSKEYDOWN:
			switch( wParam )
			{
				case VK_RETURN:
					game = (Game*)GetWindowLong( windowHandle, GWLP_USERDATA );
					game->renderer.ToggleFullscreen();

					GetScreenSpaceClientRect( windowHandle, &gClipRect );
					ClipCursor(&gClipRect);

					return 0;
					break;
			}
			break;
		case WM_SIZE:
			if( wParam == SIZE_MINIMIZED )
			{
				gWindowMinimized = true;
			}
			else if( wParam == SIZE_RESTORED && gWindowMinimized )
			{
				gWindowMinimized = false;
				GetScreenSpaceClientRect( windowHandle, &gClipRect );
				ClipCursor(&gClipRect);
			}
			return DefWindowProc( windowHandle, message, wParam, lParam );
			break;
		case WM_ACTIVATEAPP:
			if( !wParam ) // window deactivated
			{
				game = (Game*)GetWindowLong( windowHandle, GWLP_USERDATA );
				if( game->renderer.Fullscreen() ) {
					game->renderer.ToggleFullscreen();
				}
			}
			else // window activated
			{
				GetScreenSpaceClientRect( windowHandle, &gClipRect );
				ClipCursor(&gClipRect);
			}
			break;
		case WM_SETCURSOR:
			SetCursor(0);
			return 0;
			break;
		case WM_EXITSIZEMOVE:
			GetScreenSpaceClientRect( windowHandle, &gClipRect );
			ClipCursor(&gClipRect);
			if( wParam == SIZE_RESTORED )
			{
				game = (Game*)GetWindowLong( windowHandle, GWLP_USERDATA );
				if( game )
				{
					game->renderer.ResizeBuffers();
				}
				return 0;
			}
			break;
		case WM_PAINT:
			hDC = BeginPaint( windowHandle, &paintStruct );
			EndPaint( windowHandle, &paintStruct );
			break;
		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
		default:
			return DefWindowProc( windowHandle, message, wParam, lParam );
	}
	return 0;
}

bool LoadConfig()
{
	// defaults
	Config.fullscreen = FALSE;
	Config.vsync = FALSE;
	Config.screenWidth = 960;
	Config.screenHeight = 540;
	Config.multisampling = 2;
	Config.filtering = 8;
	Config.viewDistanceChunks = 12;

	const char * name = "bin/config.cfg";
	uint size = GetFileSize( name );
	if( !size )
	{
		return false;
	}

	char *configData = new char[ size ];

	if( ReadFile( name, (uint8*)configData ) != size )
	{
		delete[] configData;
		return false;
	}

	std::map<std::string, uint> configMap;

	char *line = new char[ _MAX_TOKEN_LENGTH_ ];
	char *token = new char[ _MAX_TOKEN_LENGTH_ ];
	TokenStream lines = TokenStream( configData, size );
	while( lines.NextToken( line, "" ) )
	{
		if( line[0] == ';' )
		{
			continue;
		}

		else if( line[0] == '[' )
		{
			TokenStream ts = TokenStream( line, (unsigned int)strlen( line ) );
			ts.NextToken( token, "[]" );
			OutputDebugStringA( token );
		}

		else
		{
			TokenStream ts = TokenStream( line, (unsigned int)strlen( line ) );
			ts.NextToken( token, " =" );
			std::string key( token );
			ts.NextToken( token, " =" );
			uint value = atoi( token );

			configMap[ key ] = value;
		}
	}

	delete[] token;
	token = 0;
	delete[] line;
	line = 0;
	delete[] configData;
	configData = 0;

	Config.fullscreen			= configMap[ "fullscreen" ];
	Config.vsync				= configMap[ "vsync" ];
	Config.screenWidth			= configMap[ "screenWidth" ];
	Config.screenHeight			= configMap[ "screenHeight" ];
	Config.multisampling		= configMap[ "multisampling" ];
	Config.filtering			= configMap[ "filtering" ];
	Config.viewDistanceChunks	= configMap[ "viewDistanceChunks" ];

	return true;
}

/*
NOTE:
To get the list of raw input devices on the system,
an application calls GetRawInputDeviceList.
Using the hDevice from this call, an application calls
GetRawInputDeviceInfo to get the device information.
*/

bool RegisterRawInputDevices( HWND hwnd )
{
	RAWINPUTDEVICE inputDevices[2];
	inputDevices[ 0 ].usUsagePage = 1; // generic desktop controls
	inputDevices[ 0 ].usUsage = 2; // mouse
	inputDevices[ 0 ].dwFlags = 0;
	inputDevices[ 0 ].hwndTarget = hwnd;

	inputDevices[ 1 ].usUsagePage = 1; // generic desktop controls
	inputDevices[ 1 ].usUsage = 6; // keyboard
	inputDevices[ 1 ].dwFlags = 0;
	inputDevices[ 1 ].hwndTarget = hwnd;

	if( !RegisterRawInputDevices( inputDevices, 2, sizeof( RAWINPUTDEVICE ) ) )
	{
		return false;
	}
	return true;
}

void TranslateUserInput( UserInput &userInput, LPARAM lParam )
{
	// reset mouse key states
	userInput.key[ KEY::LMB ].Pressed = false;
	userInput.key[ KEY::RMB ].Pressed = false;
	userInput.key[ KEY::MMB ].Pressed = false;
	userInput.key[ KEY::MB4 ].Pressed = false;
	userInput.key[ KEY::MB5 ].Pressed = false;
	userInput.key[ KEY::LMB ].Released = false;
	userInput.key[ KEY::RMB ].Released = false;
	userInput.key[ KEY::MMB ].Released = false;
	userInput.key[ KEY::MB4 ].Released = false;
	userInput.key[ KEY::MB5 ].Released = false;

	// get the required buffer size
	UINT size;
	GetRawInputData( (HRAWINPUT)lParam,
					 RID_INPUT,
					 NULL,
					 &size,
					 sizeof( RAWINPUTHEADER ) );

	LPBYTE buffer = new BYTE[ size ];

	UINT bytesRead = GetRawInputData( (HRAWINPUT)lParam,
									  RID_INPUT,
									  buffer,
									  &size,
									  sizeof( RAWINPUTHEADER ) );
	if( bytesRead != size )
	{
		OutputDebugStringA( "GetRawInputData returned incorrect size!" );
	}

	RAWINPUT* input = (RAWINPUT*)buffer;

	if( input->header.dwType == RIM_TYPEMOUSE )
	{
		long x = input->data.mouse.lLastX;
		long y = input->data.mouse.lLastY;

		userInput.mouse.x += x;
		userInput.mouse.y += y;

		unsigned short mouseButtons = input->data.mouse.usButtonFlags;
		// main mouse buttons
		{
			if( ( mouseButtons & RI_MOUSE_LEFT_BUTTON_DOWN ) == RI_MOUSE_LEFT_BUTTON_DOWN )
			{
				userInput.key[ KEY::LMB ].Pressed = true;
				userInput.key[ KEY::LMB ].Down = true;
			}
			if( ( mouseButtons & RI_MOUSE_LEFT_BUTTON_UP ) == RI_MOUSE_LEFT_BUTTON_UP )
			{
				userInput.key[ KEY::LMB ].Released = true;
				userInput.key[ KEY::LMB ].Down = false;
			}
			if( ( mouseButtons & RI_MOUSE_RIGHT_BUTTON_DOWN ) == RI_MOUSE_RIGHT_BUTTON_DOWN )
			{
				userInput.key[ KEY::RMB ].Pressed = true;
				userInput.key[ KEY::RMB ].Down = true;
			}
			if( ( mouseButtons & RI_MOUSE_RIGHT_BUTTON_UP ) == RI_MOUSE_RIGHT_BUTTON_UP )
			{
				userInput.key[ KEY::RMB ].Released = true;
				userInput.key[ KEY::RMB ].Down = false;
			}
			if( ( mouseButtons & RI_MOUSE_MIDDLE_BUTTON_DOWN ) == RI_MOUSE_MIDDLE_BUTTON_DOWN )
			{
				userInput.key[ KEY::MMB ].Pressed = true;
				userInput.key[ KEY::MMB ].Down = true;
			}
			if( ( mouseButtons & RI_MOUSE_MIDDLE_BUTTON_UP ) == RI_MOUSE_MIDDLE_BUTTON_UP )
			{
				userInput.key[ KEY::MMB ].Released = true;
				userInput.key[ KEY::MMB ].Down = false;
			}

		}
		// extra mouse buttons
		{
			if( ( mouseButtons & RI_MOUSE_BUTTON_4_DOWN ) == RI_MOUSE_BUTTON_4_DOWN )
			{
				userInput.key[ KEY::MB4 ].Pressed = true;
				userInput.key[ KEY::MB4 ].Down = true;
			}
			if( ( mouseButtons & RI_MOUSE_BUTTON_4_UP ) == RI_MOUSE_BUTTON_4_UP )
			{
				userInput.key[ KEY::MB4 ].Released = true;
				userInput.key[ KEY::MB4 ].Down = false;
			}
			if( ( mouseButtons & RI_MOUSE_BUTTON_5_DOWN ) == RI_MOUSE_BUTTON_5_DOWN )
			{
				userInput.key[ KEY::MB5 ].Pressed = true;
				userInput.key[ KEY::MB5 ].Down = true;
			}
			if( ( mouseButtons & RI_MOUSE_BUTTON_5_UP ) == RI_MOUSE_BUTTON_5_UP )
			{
				userInput.key[ KEY::MB5 ].Released = true;
				userInput.key[ KEY::MB5 ].Down = false;
			}
		}

		return;
	}
	else if( input->header.dwType == RIM_TYPEKEYBOARD )
	{
		unsigned short flags = input->data.keyboard.Flags;
//		unsigned short scancode = input->data.keyboard.MakeCode;
		unsigned short key = input->data.keyboard.VKey;
		unsigned short keyExtra = 0; // left or right

		bool pressed = true;

		if( ( flags & RI_KEY_BREAK ) == RI_KEY_BREAK )
		{
			pressed = false;
		}

		if( key == VK_CONTROL )
		{
			if( ( flags & RI_KEY_E0 ) == RI_KEY_E0 ) // should be E1!
			{
				keyExtra = VK_RCONTROL;
			}
			else
			{
				keyExtra = VK_LCONTROL;
			}
		}
		else if( key == VK_MENU )
		{
			if( ( flags & RI_KEY_E0 ) == RI_KEY_E0 )
			{
				keyExtra = VK_RMENU;
			}
			else
			{
				keyExtra = VK_LMENU;
			}
		}
		else if( key == VK_RETURN )
		{
			if( ( flags & RI_KEY_E0 ) == RI_KEY_E0 )
			{
				keyExtra = VK_NUMPAD_ENTER;
			}
			else
			{
				keyExtra = VK_RETURN;
			}
		}
		else if( key == VK_SHIFT )
		{
			if( input->data.keyboard.MakeCode == 0x36 )
			{
				keyExtra = VK_RSHIFT;
			}
			else
			{
				keyExtra = VK_LSHIFT;
			}
		}
		uint8 keyIndex = ( uint8 )VKeyToKey( key );
		if( userInput.key[ keyIndex ].Down == pressed )
		{
			userInput.key[ keyIndex ].Pressed = false;
			userInput.key[ keyIndex ].Released = false;
		}
		else if( !userInput.key[ keyIndex ].Down && pressed )
		{
			userInput.key[ keyIndex ].Pressed = true;
			userInput.key[ keyIndex ].Released = false;
		}
		else if( userInput.key[ keyIndex ].Down && !pressed )
		{
			userInput.key[ keyIndex ].Released = true;
			userInput.key[ keyIndex ].Pressed = false;
		}

		userInput.key[ keyIndex ].Down = pressed;

		if( keyExtra )
		{
			uint8 extraKeyIndex = ( uint8 )VKeyToKey( keyExtra );
			if( userInput.key[ extraKeyIndex ].Down == pressed )
			{
				userInput.key[ extraKeyIndex ].Pressed = false;
				userInput.key[ extraKeyIndex ].Released = false;
			}
			else if( !userInput.key[ extraKeyIndex ].Down && pressed )
			{
				userInput.key[ extraKeyIndex ].Pressed = true;
				userInput.key[ extraKeyIndex ].Released = false;
			}
			else if( userInput.key[ extraKeyIndex ].Down && !pressed )
			{
				userInput.key[ extraKeyIndex ].Released = true;
				userInput.key[ extraKeyIndex ].Pressed = false;
			}

			userInput.key[ extraKeyIndex ].Down = pressed;
		}
	}

	delete[] buffer;
}

void GetScreenSpaceClientRect( HWND hwnd, RECT * rect )
{
	GetClientRect( hwnd, rect );
	POINT min = { rect->left, rect->bottom };
	POINT max = { rect->right, rect->top };
	ClientToScreen( hwnd, &min );
	ClientToScreen( hwnd, &max );
	rect->left = min.x;
	rect->bottom = min.y;
	rect->right = max.x;
	rect->top = max.y;
}
