#ifndef __BLOCKS_UTILS__
#define __BLOCKS_UTILS__

//
// Safely releases a COM interface
//
#define RELEASE(p) if(p) { p->Release(); p = 0; }

#endif