#ifndef __OBJ_LOADER__
#define __OBJ_LOADER__

#include "../types.h"
#include "tokenstream.h"
#include <cstdlib>

int LoadObjFromMemory( uint8 * blob, uint length, float * positions, float * normals, float * texcoords );

#endif // __OBJ_LOADER__
