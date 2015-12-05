#include "win32_file.h"

HRESULT OpenFileForReading( const char *fileName, HANDLE *fileHandle );
uint GetFileSize( HANDLE file );

uint GetFileSize( const char *fileName )
{
	HANDLE file;
	HRESULT hr = OpenFileForReading( fileName, &file );
	if( FAILED( hr ) )
	{
		return 0;
	}

	uint fileSize = GetFileSize( file );
	CloseHandle( file );
	return fileSize;
}

uint ReadFile( const char *fileName, uint8 *buffer )
{
	assert( buffer );

	HANDLE file;
	HRESULT	hr = OpenFileForReading( fileName, &file );
	if( FAILED( hr ) )
	{
		return 0;
	}

	uint fileSize = GetFileSize( file );
	if( !fileSize )
	{
		return 0;
	}

	DWORD bytesRead = 0;
    if( !ReadFile( file, buffer, fileSize, &bytesRead, nullptr ) )
    {
        return 0;
    }
    if( bytesRead < fileSize )
    {
        return 0;
    }

	CloseHandle( file );

    return fileSize;
}

uint GetFileSize( HANDLE file )
{
	LARGE_INTEGER fSize;
	if( !GetFileSizeEx( file, &fSize ) )
	{
		return 0;
	}

	// NOTE: don't bother with files larger than 4GB by now
	if( fSize.HighPart > 0 )
	{
		return 0;
	}

	return fSize.LowPart;
}

HRESULT OpenFileForReading( const char *fileName, HANDLE *fileHandle )
{
	HANDLE fHandle =  CreateFile(
									fileName,
									GENERIC_READ,
									0, // NOTE: may need shared access in the future
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL
								);
	if( fHandle == INVALID_HANDLE_VALUE )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	*fileHandle = fHandle;

	return S_OK;
}
