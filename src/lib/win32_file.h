#ifndef __WIN32_FILE__
#define __WIN32_FILE__

#include <Windows.h>
#include <assert.h>
#include "../types.h"

uint GetFileSize( const char *fileName );
uint ReadFile( const char *fileName, uint8 *buffer );

#endif // __WIN32_FILE__
