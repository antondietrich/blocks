#ifndef __ASSET_DEF__
#define __ASSET_DEF__

enum TEXTURE_TYPE
{
	SINGLE,
	ARRAY,
};

enum TEXTURE
{
	BLOCKS_OPAQUE,
	BLOCKS_TRANSPARENT,
	LIGHT_COLOR,
	FONT,
	CROSSHAIR,

	COUNT
};

enum BLOCK_TEXTURE
{
	GRASS,
	DIRT,
	ROCK,
	WOOD,
	TREE_BARK,
	TREE_TRUNK,
	LEAVES,

	BLOCK_TEXTURE_COUNT
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

void LoadTextureDefinitions( TextureDefinition * textureDefinitions );
void FreeTextureDefinitions( TextureDefinition * textureDefinitions );

#ifdef ASSET_DEF_IMPLEMENTATION

void LoadTextureDefinitions( TextureDefinition * textureDefinitions )
{
	for( int i = 0; i < TEXTURE::COUNT; i++ )
	{
		textureDefinitions[ i ].type = (TEXTURE_TYPE)0;
		textureDefinitions[ i ].arraySize = 0;
		textureDefinitions[ i ].width = 0;
		textureDefinitions[ i ].height = 0;
		textureDefinitions[ i ].arraySize = 0;
		textureDefinitions[ i ].filenames = 0;
	}

	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].type = TEXTURE_TYPE::ARRAY;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].arraySize = BLOCK_TEXTURE_COUNT;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].width = 512;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].height = 512;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].generateMips = true;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames = new char*[ BLOCK_TEXTURE_COUNT ];
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[0] = "assets/textures/grass.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[1] = "assets/textures/dirt.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[2] = "assets/textures/rock.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[3] = "assets/textures/wood.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[4] = "assets/textures/tree_bark.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[5] = "assets/textures/wood_trunk.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[6] = "assets/textures/leaves.png";

	textureDefinitions[ TEXTURE::LIGHT_COLOR ].type = TEXTURE_TYPE::SINGLE;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].arraySize = 1;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].width = 256;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].height = 8;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].generateMips = false;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].filenames = new char*[ 1 ];
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].filenames[0] = "assets/textures/light_color.png";

	textureDefinitions[ TEXTURE::FONT ].type = TEXTURE_TYPE::SINGLE;
	textureDefinitions[ TEXTURE::FONT ].arraySize = 1;
	textureDefinitions[ TEXTURE::FONT ].width = 1024;
	textureDefinitions[ TEXTURE::FONT ].height = 24;
	textureDefinitions[ TEXTURE::FONT ].generateMips = false;
	textureDefinitions[ TEXTURE::FONT ].filenames = new char*[ 1 ];
	textureDefinitions[ TEXTURE::FONT ].filenames[0] = "assets/textures/droid_mono.png";

	textureDefinitions[ TEXTURE::CROSSHAIR ].type = TEXTURE_TYPE::SINGLE;
	textureDefinitions[ TEXTURE::CROSSHAIR ].arraySize = 1;
	textureDefinitions[ TEXTURE::CROSSHAIR ].width = 16;
	textureDefinitions[ TEXTURE::CROSSHAIR ].height = 8;
	textureDefinitions[ TEXTURE::CROSSHAIR ].generateMips = false;
	textureDefinitions[ TEXTURE::CROSSHAIR ].filenames = new char*[ 1 ];
	textureDefinitions[ TEXTURE::CROSSHAIR ].filenames[0] = "assets/textures/crosshair.png";
}

void FreeTextureDefinitions( TextureDefinition * textureDefinitions )
{
	for( int i = 0; i < TEXTURE::COUNT; i++ )
	{
		if( textureDefinitions[ i ].filenames )
		{
			delete[] textureDefinitions[ i ].filenames;
		}
	}
}

#endif ASSET_DEF_IMPLEMENTATION

#endif // __ASSET_DEF__
