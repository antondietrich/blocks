#ifndef __ASSET_DEF__
#define __ASSET_DEF__

enum TEXTURE
{
	BLOCKS_OPAQUE,
	BLOCKS_TRANSPARENT,
	LIGHT_COLOR,
	FONT,

	COUNT
};

enum TEXTURE_TYPE
{
	SINGLE,
	ARRAY,
};

struct TextureDefinition
{
	char ** filenames;
	TEXTURE_TYPE type;
	uint arraySize;
	uint width;
	uint height;
	bool generateMips;
};

// extern TextureDefinition gTextureDefinitions[ TEXTURE::COUNT ];

#endif // __ASSET_DEF__
