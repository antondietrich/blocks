#include <Windows.h>

#include "game.h"

LRESULT CALLBACK WndProc( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam );

int __stdcall wWinMain( HINSTANCE thisInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow )
{
	UNREFERENCED_PARAMETER( prevInstance );
	UNREFERENCED_PARAMETER( cmdLine );

	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof( WNDCLASSEX );
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WndProc;
	windowClass.hInstance = thisInstance;
	windowClass.hIcon = LoadIcon(thisInstance, MAKEINTRESOURCE(IDI_APPLICATION));;
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
	if( Config.screenWidth == 0 || Config.screenHeight == 0 )
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

	HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to show main window!" );
		return 1;
	}


	MSG msg = { 0 };
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}

	return (int) msg.wParam;
}

LRESULT CALLBACK WndProc( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT paintStruct;
	HDC hDC;
	switch( message )
	{
		case WM_KEYDOWN:
			switch( wParam )
			{
				case VK_ESCAPE:
					PostQuitMessage( 0 );
					break;
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