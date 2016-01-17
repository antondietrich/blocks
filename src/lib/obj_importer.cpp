#include "obj_importer.h"

struct FaceVertex
{
	int vIndex_;
	int nIndex_;
	int tIndex_;
};

struct Face
{
	FaceVertex v1_;
	FaceVertex v2_;
	FaceVertex v3_;
};

int LoadObjFromMemory( uint8 * blob, uint length, float * positions, float * normals, float * texcoords )
{
	int vertexCount = 0;

	int numVertices = 0, numNormals = 0, numTexCoords = 0, numFaces = 0;
	char *line = new char[ _MAX_TOKEN_LENGTH_ ];
	char *token = new char[ _MAX_TOKEN_LENGTH_ ];
	char *tmp = new char[ _MAX_TOKEN_LENGTH_ ];

	TokenStream fileTokenStream = TokenStream( (char*)blob, length );
	TokenStream lineTokenStream;
	TokenStream faceTokenStream;

	// get primitive counts
	while( fileTokenStream.NextToken( line ) )
	{
		lineTokenStream.SetSource( line );
		lineTokenStream.NextToken( token, " " );
		if( strcmp( token, "#" ) == 0 )
		{
			lineTokenStream.NextToken( tmp, " " );
			lineTokenStream.NextToken( token, " " );
			if( strcmp( token, "vertices" ) == 0 )
			{
				numVertices = atoi( tmp );
			}
			else if( strcmp( token, "vertex" ) == 0 )
			{
				numNormals = atoi( tmp );
			}
			else if( strcmp( token, "texture" ) == 0 )
			{
				numTexCoords = atoi( tmp );
			}
			else if( strcmp( token, "faces" ) == 0 )
			{
				numFaces = atoi( tmp );
			}
		}
	}
	fileTokenStream.Reset();

	vertexCount = numFaces * 3;

	int vIndex = 0, nIndex = 0, tIndex = 0, fIndex = 0;

	float	*tempPositions 	=	new float[ numVertices * 3 ],
			*tempNormals 	=	new float[ numNormals * 3 ],
			*tempTexcoords 	=	new float[ numTexCoords * 2 ];
	Face	*faces 		=	new Face[ numFaces ];

	// get raw data
	while( fileTokenStream.NextToken( line ) )
	{
		lineTokenStream.SetSource( line );
		lineTokenStream.NextToken( token, " " );
		if( strcmp( token, "v" ) == 0 )
		{
			lineTokenStream.NextToken( token, " " );
			tempPositions[ vIndex++ ] = strtof( token, nullptr );
			lineTokenStream.NextToken( token, " " );
			tempPositions[ vIndex++ ] = strtof( token, nullptr );
			lineTokenStream.NextToken( token, " " );
			tempPositions[ vIndex++ ] = strtof( token, nullptr );
		}
		else if( strcmp( token, "vn" ) == 0 )
		{
			lineTokenStream.NextToken( token, " " );
			tempNormals[ nIndex++ ] = strtof( token, nullptr );
			lineTokenStream.NextToken( token, " " );
			tempNormals[ nIndex++ ] = strtof( token, nullptr );
			lineTokenStream.NextToken( token, " " );
			tempNormals[ nIndex++ ] = strtof( token, nullptr );
		}
		else if( strcmp( token, "vt" ) == 0 )
		{
			lineTokenStream.NextToken( token, " " );
			tempTexcoords[ tIndex++ ] = strtof( token, nullptr );
			lineTokenStream.NextToken( token, " " );
			tempTexcoords[ tIndex++ ] = strtof( token, nullptr );
		}
		else if( strcmp( token, "f" ) == 0 )
		{
			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v1_.vIndex_ = atoi( token ) - 1;
			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v1_.tIndex_ = atoi( token ) - 1;
			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v1_.nIndex_ = atoi( token ) - 1;

			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v2_.vIndex_ = atoi( token ) - 1;
			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v2_.tIndex_ = atoi( token ) - 1;
			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v2_.nIndex_ = atoi( token ) - 1;

			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v3_.vIndex_ = atoi( token ) - 1;
			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v3_.tIndex_ = atoi( token ) - 1;
			lineTokenStream.NextToken( token, " /" );
			faces[ fIndex ].v3_.nIndex_ = atoi( token ) - 1;

			++fIndex;
		}
	}

	//positions = new float[ vertexCount * 3 ];
	//normals = new float[ vertexCount * 3 ];
	//texcoords = new float[ vertexCount * 2 ];

	for( int i = 0; i < numFaces; i++ )
	{
		// vertex 1
		#if FLIP_WINDING_ORDER

		positions[ i * 9 + 0 ] = tempPositions[ faces[ i ].v1_.vIndex_ * 3 + 0 ];
		positions[ i * 9 + 1 ] = tempPositions[ faces[ i ].v1_.vIndex_ * 3 + 1 ];
		positions[ i * 9 + 2 ] = tempPositions[ faces[ i ].v1_.vIndex_ * 3 + 2 ];

		positions[ i * 9 + 3 ] = tempPositions[ faces[ i ].v3_.vIndex_ * 3 + 0 ];
		positions[ i * 9 + 4 ] = tempPositions[ faces[ i ].v3_.vIndex_ * 3 + 1 ];
		positions[ i * 9 + 5 ] = tempPositions[ faces[ i ].v3_.vIndex_ * 3 + 2 ];

		positions[ i * 9 + 6 ] = tempPositions[ faces[ i ].v2_.vIndex_ * 3 + 0 ];
		positions[ i * 9 + 7 ] = tempPositions[ faces[ i ].v2_.vIndex_ * 3 + 1 ];
		positions[ i * 9 + 8 ] = tempPositions[ faces[ i ].v2_.vIndex_ * 3 + 2 ];

		normals[ i * 9 + 0 ] = tempNormals[ faces[ i ].v1_.nIndex_ * 3 + 0 ];
		normals[ i * 9 + 1 ] = tempNormals[ faces[ i ].v1_.nIndex_ * 3 + 1 ];
		normals[ i * 9 + 2 ] = tempNormals[ faces[ i ].v1_.nIndex_ * 3 + 2 ];
		normals[ i * 9 + 3 ] = tempNormals[ faces[ i ].v3_.nIndex_ * 3 + 0 ];
		normals[ i * 9 + 4 ] = tempNormals[ faces[ i ].v3_.nIndex_ * 3 + 1 ];
		normals[ i * 9 + 5 ] = tempNormals[ faces[ i ].v3_.nIndex_ * 3 + 2 ];
		normals[ i * 9 + 6 ] = tempNormals[ faces[ i ].v2_.nIndex_ * 3 + 0 ];
		normals[ i * 9 + 7 ] = tempNormals[ faces[ i ].v2_.nIndex_ * 3 + 1 ];
		normals[ i * 9 + 8 ] = tempNormals[ faces[ i ].v2_.nIndex_ * 3 + 2 ];

		texcoords[ i * 6 + 0 ] =  tempTexcoords[ faces[ i ].v1_.tIndex_ * 2 + 0 ];
		texcoords[ i * 6 + 1 ] =  1.0f - (tempTexcoords[ faces[ i ].v1_.tIndex_ * 2 + 1 ]);
		texcoords[ i * 6 + 2 ] =  tempTexcoords[ faces[ i ].v3_.tIndex_ * 2 + 0 ];
		texcoords[ i * 6 + 3 ] =  1.0f - (tempTexcoords[ faces[ i ].v3_.tIndex_ * 2 + 1 ]);
		texcoords[ i * 6 + 4 ] =  tempTexcoords[ faces[ i ].v2_.tIndex_ * 2 + 0 ];
		texcoords[ i * 6 + 5 ] =  1.0f - (tempTexcoords[ faces[ i ].v2_.tIndex_ * 2 + 1 ]);

		#else
		positions[ i * 9 + 0 ] = tempPositions[ faces[ i ].v1_.vIndex_ * 3 + 0 ];
		positions[ i * 9 + 1 ] = tempPositions[ faces[ i ].v1_.vIndex_ * 3 + 1 ];
		positions[ i * 9 + 2 ] = tempPositions[ faces[ i ].v1_.vIndex_ * 3 + 2 ];

		positions[ i * 9 + 3 ] = tempPositions[ faces[ i ].v2_.vIndex_ * 3 + 0 ];
		positions[ i * 9 + 4 ] = tempPositions[ faces[ i ].v2_.vIndex_ * 3 + 1 ];
		positions[ i * 9 + 5 ] = tempPositions[ faces[ i ].v2_.vIndex_ * 3 + 2 ];

		positions[ i * 9 + 6 ] = tempPositions[ faces[ i ].v3_.vIndex_ * 3 + 0 ];
		positions[ i * 9 + 7 ] = tempPositions[ faces[ i ].v3_.vIndex_ * 3 + 1 ];
		positions[ i * 9 + 8 ] = tempPositions[ faces[ i ].v3_.vIndex_ * 3 + 2 ];

		normals[ i * 9 + 0 ] = tempNormals[ faces[ i ].v1_.nIndex_ * 3 + 0 ];
		normals[ i * 9 + 1 ] = tempNormals[ faces[ i ].v1_.nIndex_ * 3 + 1 ];
		normals[ i * 9 + 2 ] = tempNormals[ faces[ i ].v1_.nIndex_ * 3 + 2 ];
		normals[ i * 9 + 3 ] = tempNormals[ faces[ i ].v2_.nIndex_ * 3 + 0 ];
		normals[ i * 9 + 4 ] = tempNormals[ faces[ i ].v2_.nIndex_ * 3 + 1 ];
		normals[ i * 9 + 5 ] = tempNormals[ faces[ i ].v2_.nIndex_ * 3 + 2 ];
		normals[ i * 9 + 6 ] = tempNormals[ faces[ i ].v3_.nIndex_ * 3 + 0 ];
		normals[ i * 9 + 7 ] = tempNormals[ faces[ i ].v3_.nIndex_ * 3 + 1 ];
		normals[ i * 9 + 8 ] = tempNormals[ faces[ i ].v3_.nIndex_ * 3 + 2 ];

		texcoords[ i * 6 + 0 ] =  tempTexcoords[ faces[ i ].v1_.tIndex_ * 2 + 0 ];
		texcoords[ i * 6 + 1 ] = -tempTexcoords[ faces[ i ].v1_.tIndex_ * 2 + 1 ];
		texcoords[ i * 6 + 2 ] =  tempTexcoords[ faces[ i ].v2_.tIndex_ * 2 + 0 ];
		texcoords[ i * 6 + 3 ] = -tempTexcoords[ faces[ i ].v2_.tIndex_ * 2 + 1 ];
		texcoords[ i * 6 + 4 ] =  tempTexcoords[ faces[ i ].v3_.tIndex_ * 2 + 0 ];
		texcoords[ i * 6 + 5 ] = -tempTexcoords[ faces[ i ].v3_.tIndex_ * 2 + 1 ];
		#endif
	}

	delete[] line;
	line = 0;
	delete[] token;
	token = 0;
	delete[] tmp;
	tmp = 0;

	delete[] tempPositions;
	tempPositions = 0;
	delete[] tempNormals;
	tempNormals = 0;
	delete[] tempTexcoords;
	tempTexcoords = 0;
	delete[] faces;
	faces = 0;

	return vertexCount;
}
