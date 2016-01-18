#ifndef __OBJ_LOADER__
#define __OBJ_LOADER__

#include "../types.h"
#include "win32_file.h"
#include "tokenstream.h"
#include <cstdlib>

#define FLIP_WINDING_ORDER 1

#define MAX_VERTS_PER_MESH (4096 * 3)

int LoadObjFromFile( const char * filename, float * positions, float * normals, float * texcoords );
int LoadObjFromMemory( uint8 * blob, uint length, float * positions, float * normals, float * texcoords );

#endif // __OBJ_LOADER__
