#ifndef __BLOCK_DEF__
#define __BLOCK_DEF__

enum BLOCK_TYPE
{
	BT_AIR,
	BT_DIRT,
	BT_GRASS,
	BT_STONE,
	BT_WOOD,
	BT_TREE_TRUNK,
	BT_TREE_LEAVES,

	BT_COUNT
};

enum BLOCK_FACE_DIRECTION
{
	BFD_SIDE,
	BFD_TOP
};

struct BlockDefinition
{
	int textureID[2];
	int meshID;
	int transparent;
};

extern BlockDefinition blockDefinitions[ BT_COUNT ];

#ifdef BLOCK_DEF_IMPLEMENTATION

BlockDefinition blockDefinitions[ BT_COUNT ] =
{
	{ { 0, 0 }, 0, 1 },
	{ { BLOCK_TEXTURE::DIRT,		BLOCK_TEXTURE::DIRT },			0, 		0 },
	{ { BLOCK_TEXTURE::GRASS,		BLOCK_TEXTURE::GRASS },			0, 		0 },
	{ { BLOCK_TEXTURE::ROCK,		BLOCK_TEXTURE::ROCK },			0, 		0 },
	{ { BLOCK_TEXTURE::WOOD,		BLOCK_TEXTURE::WOOD },			0, 		0 },
	{ { BLOCK_TEXTURE::TREE_BARK,	BLOCK_TEXTURE::TREE_TRUNK },	0, 		0 },
	{ { BLOCK_TEXTURE::LEAVES,		BLOCK_TEXTURE::LEAVES },		0, 		1 },
};

//blockDefinitions[ BT_AIR ]				= {0, 0};
//blockDefinitions[ BT_DIRT ]				= {BLOCK_TEXTURE::DIRT, 0};
//blockDefinitions[ BT_GRASS ]			= {BLOCK_TEXTURE::GRASS, 0};
//blockDefinitions[ BT_STONE ]			= {BLOCK_TEXTURE::ROCK, 0};
//blockDefinitions[ BT_WOOD ]				= {BLOCK_TEXTURE::WOOD, 0};
//blockDefinitions[ BT_TREE_TRUNK ]		= {BLOCK_TEXTURE::TREE_BARK, 0};
//blockDefinitions[ BT_TREE_LEAVES ]		= {BLOCK_TEXTURE::TREE_LEAVES, 0};

#endif

#endif // __BLOCK_DEF__
