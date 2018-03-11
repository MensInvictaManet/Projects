/*****************************************************************/
/*	           ___                          _  _                 */
/*            / _ \                        | |(_)                */
/*           / /_\ \ _ __   ___   __ _   __| | _   __ _          */
/*           |  _  || '__| / __| / _` | / _` || | / _` |         */
/*	         | | | || |   | (__ | (_| || (_| || || (_| |         */
/*	         \_| |_/|_|    \___| \__,_| \__,_||_| \__,_|         */
/*                                                               */
/*                                      Engine Version 01.00.00  */
/*****************************************************************/
/*  File:       WorldController.cpp                              */
/*                                                               */
/*  Purpose:    This file contains a class that will load in and */
/*              manage a 3D map which can then be rendered and   */
/*              polled for information. This should allow for    */
/*              very simple terrain management in a 3D game.     */
/*                                                               */
/*  Created:    12/07/2008                                       */
/*  Last Edit:  04/18/2009                                       */
/*****************************************************************/

#include "WorldController.h"

#include "Collision/LineToTriangle.h"
#include "DebugTools.h"

WorldController* WorldController::Get_Instance( void )
{
	static WorldController INSTANCE;
	return &INSTANCE;
}


bool WorldController::Load_Map_Data_Struct( const char* world_geometry_file, MapDataStruct& data_struct )
{
	//  Load the map geometry data from XML
	const RapidXML_Doc* worldDoc = XMLWrapper::Get_Instance()->LoadXMLFile( world_geometry_file );
	if ( !worldDoc ) { return false; }
	
	const RapidXML_Node* mainNode = worldDoc->first_node();
	FAIL_IF ( mainNode == NULL ) return false;

	// The entire XML should be composed of four tags (Rows, Columns, HeightList, and ColorPalette)
	const RapidXML_Node* rowsNode = mainNode->first_node();
	FAIL_IF ( rowsNode == NULL || strcmp( rowsNode->name(), "Rows" ) != 0 )									{ return false; }

	const RapidXML_Node* columnsNode = rowsNode->next_sibling();
	FAIL_IF ( columnsNode == NULL || strcmp( columnsNode->name(), "Columns" ) != 0 )						{ return false; }

	const RapidXML_Node* heightListNode = columnsNode->next_sibling();
	FAIL_IF ( heightListNode == NULL || strcmp( heightListNode->name(), "HeightList" ) != 0 )			{ return false; }

	const RapidXML_Node* colorPaletteNode = heightListNode->next_sibling();
	FAIL_IF ( colorPaletteNode == NULL || strcmp( colorPaletteNode->name(), "ColorPalette" ) != 0 )	{ return false; }

	data_struct.Width = atoi( rowsNode->value() );
	data_struct.Height = atoi( columnsNode->value() );

	const int heightCount = data_struct.Width * data_struct.Height;
	data_struct.HeightList = new float[heightCount];

	int heightsRead = 0;
	for ( const RapidXML_Node* heightIter = heightListNode->first_node(); heightIter != NULL; heightIter = heightIter->next_sibling() )
	{
		data_struct.HeightList[heightsRead++] = float(atof( heightIter->value() ));
	}

	Load_Color_Palette( data_struct, colorPaletteNode->value() );

	//  We have the information we need. Close/Delete the XML file
	XMLWrapper::Get_Instance()->RemoveXMLFile( world_geometry_file );

	return true;
}


bool WorldController::Load_World_Geometry( const char* world_geometry_file )
{
	//  Load the map geometry data from XML
	const RapidXML_Doc* worldDoc = XMLWrapper::Get_Instance()->LoadXMLFile( world_geometry_file );
	if ( !worldDoc ) { return false; }
	
	const RapidXML_Node* mainNode = worldDoc->first_node();
	FAIL_IF ( mainNode == NULL ) return NULL;

	// The entire XML should be composed of four tags (Rows, Columns, HeightList, and ColorPalette)
	const RapidXML_Node* rowsNode = mainNode->first_node();
	FAIL_IF ( rowsNode == NULL || strcmp(rowsNode->name(), "Rows" ) != 0 )									{ return false; }

	const RapidXML_Node* columnsNode = rowsNode->next_sibling();
	FAIL_IF ( columnsNode == NULL || strcmp(columnsNode->name(), "Columns" ) != 0 )						{ return false; }

	const RapidXML_Node* heightListNode = columnsNode->next_sibling();
	FAIL_IF ( heightListNode == NULL || strcmp(heightListNode->name(), "HeightList" ) != 0 )			{ return false; }

	const RapidXML_Node* colorPaletteNode = heightListNode->next_sibling();
	FAIL_IF ( colorPaletteNode == NULL || strcmp(colorPaletteNode->name(), "ColorPalette" ) != 0 )	{ return false; }

	MapDataStruct data_struct;

	data_struct.Width = atoi( rowsNode->value() );
	data_struct.Height = atoi( columnsNode->value() );

	const int heightCount = data_struct.Width * data_struct.Height;
	data_struct.HeightList = new float[heightCount];

	int heightsRead = 0;
	for ( const RapidXML_Node* heightIter = heightListNode->first_node(); heightIter != NULL; heightIter = heightIter->next_sibling() )
	{
		data_struct.HeightList[heightsRead++] = float(atof( heightIter->value() ));
	}

	Load_Color_Palette( data_struct, colorPaletteNode->value() );

	//  We have the information we need. Close/Delete the XML file
	XMLWrapper::Get_Instance()->RemoveXMLFile( world_geometry_file );

	return Load_World_Geometry( data_struct );
}


bool WorldController::Load_World_Geometry( MapDataStruct& data_struct )
{
	/*
	//  If we've gotten this far, we are prepared to delete the old geometry data and write new data
	glDeleteLists( DrawList, 1 );

	delete [] VertexList;
	delete [] TriangleList;

	// Create data based on the lengths
	VertexCount		= ( (data_struct.Width + 1) * (data_struct.Height + 1) ) + data_struct.Width * data_struct.Height;
	TriangleCount	= data_struct.Width * data_struct.Height * 4;
	Rows = data_struct.Width;
	Columns = data_struct.Height;
	MapXLength		= SPACE_BETWEEN_POINTS_X * float(data_struct.Width);
	MapZLength		= SPACE_BETWEEN_POINTS_Z * float(data_struct.Height);
	DrawList = glGenLists( 1 );

	VertexList = new Vertex[VertexCount];
	TriangleList = new Triangle[TriangleCount];

	//  Set the highest and lowest points to an arbitrary height in the height map (to ensure that they don't go above or below the true high and low points)
	LowestVertexYValue = data_struct.HeightList[0];
	HighestVertexYValue = data_struct.HeightList[0];

	//  For each of the vertices we DO have heights for, define the positions
	unsigned int middle_vertex_list_offset = ((data_struct.Width + 1) * (data_struct.Height + 1));
	for ( unsigned int i = 0; i < data_struct.Width * data_struct.Height; ++i )
	{
		VertexList[i + middle_vertex_list_offset].Define_Vertex((i % data_struct.Width) * SPACE_BETWEEN_POINTS_X + (SPACE_BETWEEN_POINTS_X / 2.0f), data_struct.HeightList[i], (i / data_struct.Width) * SPACE_BETWEEN_POINTS_Z + (SPACE_BETWEEN_POINTS_Z / 2.0f));

		//  Update the lowest and highest values on the map
		if ( data_struct.HeightList[i] < LowestVertexYValue ) { LowestVertexYValue = data_struct.HeightList[i]; }
		if ( data_struct.HeightList[i] > HighestVertexYValue ) { HighestVertexYValue = data_struct.HeightList[i]; }
	}

	//  For each of the vertices we DON'T have heights for (the outer vertices for each square), define the height
	unsigned int surrounding_vertex_count;
	float averaged_height;
	for ( unsigned int i = 0; i < data_struct.Height + 1; ++i )
	{
		for ( unsigned int j = 0; j < data_struct.Width + 1; ++j )
		{
			surrounding_vertex_count = 0;
			averaged_height = 0.0f;

			if ( j > 0 ) //  If we are not on the left wall
			{
				if ( i > 0 ) //  If we are not on the bottom wall
				{
					++surrounding_vertex_count;
					averaged_height += data_struct.HeightList[(j - 1) + (data_struct.Width * (i - 1))];
				}
				
				if ( i < data_struct.Height ) //  If we are not on the top wall
				{
					++surrounding_vertex_count;
					averaged_height += data_struct.HeightList[(j - 1) + (data_struct.Width * i)];
				}
			}
			
			if ( j < data_struct.Width ) //  If we are not on the right wall
			{
				if ( i > 0 ) //  If we are not on the bottom wall
				{
					++surrounding_vertex_count;
					averaged_height += data_struct.HeightList[j + (data_struct.Width * (i - 1))];
				}

				if ( i < data_struct.Height ) //  If we are not on the top wall
				{
					++surrounding_vertex_count;
					averaged_height += data_struct.HeightList[j + (data_struct.Width * i)];
				}
			}

			//  Determine the average height of the surrounding vertices
			averaged_height /= float(surrounding_vertex_count);

			//  Define the vertex
			VertexList[j + ((data_struct.Width + 1) * i)].Define_Vertex( j * SPACE_BETWEEN_POINTS_X, averaged_height, i * SPACE_BETWEEN_POINTS_Z );

			//  Update the lowest and highest values on the map
			if ( averaged_height < LowestVertexYValue ) { LowestVertexYValue = averaged_height; }
			if ( averaged_height > HighestVertexYValue ) { HighestVertexYValue = averaged_height; }
		}
	}

	//  Define all of the triangles in our world geometry
	for ( unsigned int i = 0; i < data_struct.Width * data_struct.Height; ++i )
	{
		unsigned int row = i % data_struct.Width;
		unsigned int column = i / data_struct.Width;
		unsigned int triangle_index = i * 4;

		VertexIndexType v0;
		VertexIndexType v1;
		VertexIndexType v2;

		// Triangle 1 out of 4
		v0 = ((row + 0) + (data_struct.Width + 1) * (column + 0));
		v1 = ((data_struct.Width + 1) * (data_struct.Height + 1)) + i;
		v2 = ((row + 0) + (data_struct.Width + 1) * (column + 1));
		TriangleList[triangle_index + 0].Define_Triangle( v0, v1, v2 );

		// Triangle 2 out of 4
		v0 = ((row + 0) + (data_struct.Width + 1) * (column + 0));
		v1 = ((row + 1) + (data_struct.Width + 1) * (column + 0));
		v2 = ((data_struct.Width + 1) * (data_struct.Height + 1)) + i;
		TriangleList[triangle_index + 1].Define_Triangle( v0, v1, v2 );

		// Triangle 3 out of 4
		v0 = ((row + 1) + (data_struct.Width + 1) * (column + 0));
		v1 = ((row + 1) + (data_struct.Width + 1) * (column + 1));
		v2 = ((data_struct.Width + 1) * (data_struct.Height + 1)) + i;
		TriangleList[triangle_index + 2].Define_Triangle( v0, v1, v2 );

		// Triangle 4 out of 4
		v0 = ((row + 0) + (data_struct.Width + 1) * (column + 1));
		v1 = ((data_struct.Width + 1) * (data_struct.Height + 1)) + i;
		v2 = ((row + 1) + (data_struct.Width + 1) * (column + 1));
		TriangleList[triangle_index + 3].Define_Triangle( v0, v1, v2 );
	}

	float palette_modifier = (( data_struct.Palette->Width != 0) ? (HighestVertexYValue - LowestVertexYValue) / (float)data_struct.Palette->Width : 1.0f );
	unsigned int palette_row;
	unsigned int palette_column;
	unsigned int color_index;
	float triangle_elevation;

	// Create the Draw List for the basic geometry
	glNewList( DrawList, GL_COMPILE );
		glBegin(GL_TRIANGLES);
		for ( unsigned int i = 0; i < TriangleCount; ++i )
		{
			if ( data_struct.Palette->Colors != 0 )
			{
				triangle_elevation = 0.0f;
				for (unsigned int j = 0; j < 3; ++j)
				{
					triangle_elevation += VertexList[TriangleList[i].VertexIndexList.array()[j]].Position.y;
				}
				triangle_elevation /= 3.0f;
				
				palette_row = (unsigned int)((triangle_elevation - LowestVertexYValue) / palette_modifier);
				palette_column = (unsigned int)(sqrt((Random_Float(0.0f, float(data_struct.Palette->Height * data_struct.Palette->Height - 1)))));
				color_index = (palette_column * data_struct.Palette->Width + palette_row);
				glColor3f( data_struct.Palette->Colors[color_index].x, data_struct.Palette->Colors[color_index].y, data_struct.Palette->Colors[color_index].z );
			}

			for ( unsigned int j = 0; j < 3; ++j )
			{
				unsigned int index = TriangleList[i].VertexIndexList.array()[j];
				glNormal3f(VertexList[index].Normal.x, VertexList[index].Normal.y, VertexList[index].Normal.z);
				glTexCoord2d(VertexList[index].Position.x / MapXLength, VertexList[index].Position.z / MapZLength);
				glVertex3f(VertexList[index].Position.x, VertexList[index].Position.y, VertexList[index].Position.z);		
			}
		}
		glEnd();
	glEndList();
	*/
	return true;
}


bool WorldController::Load_Color_Palette( MapDataStruct& data_struct, const char* color_palette_file )
{
	//  Load the color palette data from XML
	const RapidXML_Doc* paletteDoc = XMLWrapper::Get_Instance()->LoadXMLFile( color_palette_file );
	if ( !paletteDoc ) { return false; }
	
	const RapidXML_Node* mainNode = paletteDoc->first_node();
	FAIL_IF ( mainNode == NULL ) return false;
	
	// The entire XML should be composed of three tags (Width, Height, and Palette)
	const RapidXML_Node* widthNode = mainNode->first_node();
	FAIL_IF ( widthNode == NULL || strcmp(widthNode->name(), "Width" ) != 0 )					{ return false; }

	const RapidXML_Node* heightNode = widthNode->next_sibling();
	FAIL_IF ( heightNode == NULL || strcmp(heightNode->name(), "Height" ) != 0 )				{ return false; }

	const RapidXML_Node* paletteNode = heightNode->next_sibling();
	FAIL_IF ( paletteNode == NULL || strcmp(paletteNode->name(), "Palette" ) != 0 )			{ return false; }

	unsigned int width = atoi( widthNode->value() );
	unsigned int height = atoi( heightNode->value() );

	for ( const RapidXML_Node* colorIter = paletteNode->first_node(); colorIter != NULL; colorIter = colorIter->next_sibling() )
	{
		if ( colorIter->first_node( "R" ) == NULL	)	{ return false; }
		if ( colorIter->first_node( "G" ) == NULL	)	{ return false; }
		if ( colorIter->first_node( "R" ) == NULL	)	{ return false; }
	}

	delete [] data_struct.Palette;
	data_struct.Palette = new ColorPalette;

	data_struct.Palette->Width = width;
	data_struct.Palette->Height = height;
	data_struct.Palette->Colors = new Vector3<float>[data_struct.Palette->Width * data_struct.Palette->Height];
	int colorIndex = 0;
	for ( const RapidXML_Node* colorIter = paletteNode->first_node(); colorIter != NULL; colorIter = colorIter->next_sibling() )
	{
		data_struct.Palette->Colors[colorIndex++] = Vector3<float>( float(atof( colorIter->first_node( "R" )->value() )), float(atof( colorIter->first_node( "G" )->value() )), float(atof( colorIter->first_node( "B" )->value() )) );
	}

	//  We have the information we need. Close/Delete the XML file
	XMLWrapper::Get_Instance()->RemoveXMLFile( color_palette_file );

	return true;
}


void WorldController::Render_Map( void ) const
{
	/*
	//  Set the fill the geometry when rendering
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	// Render the geometry (triangles)
	glDisable(GL_TEXTURE_2D);
	glCallList( DrawList );
	*/
}


bool WorldController::Get_Height_At_Position( Vector3<float>& position ) const
{
	for ( unsigned int i = 0; i < TriangleCount; ++i )
	{
		Triangle& triangle = TriangleList[i];
	}

	return true;
}


bool WorldController::Ray_Collision( const Vector3<float>& line_start, const Vector3<float>& line_end, Vector3<float>& intersect_position ) const
{
	for ( unsigned int i = 0; i < TriangleCount; ++i )
	{
		Triangle& triangle = TriangleList[i];
		BasicTriangle basic_triangle( VertexList[triangle.VertexIndexList.x].Position, VertexList[triangle.VertexIndexList.y].Position, VertexList[triangle.VertexIndexList.z].Position );

		if ( LineToTriangle3D( intersect_position, basic_triangle, line_start, line_end ) == true ) { return true; } 
	}

   return  false;
}


bool WorldController::Colliding_Square( const Vector3<float>& line_start, const Vector3<float>& line_end, std::pair< int, int >& colliding_square ) const
{
	Vector3<float> intersect_position;
	if ( !Ray_Collision( line_start, line_end, intersect_position ) )
	{
		return false;
	}

	//  Determine the square based on intersection position
	for ( unsigned int i = 0; i < Rows; ++i )
	{
		for ( unsigned int j = 0; j < Columns; ++j )
		{
			//  Check if the position is within the X and Y of the square for the column and row
			if ( intersect_position.x < float(SPACE_BETWEEN_POINTS_X) * float(i) )												{ continue; }
			if ( intersect_position.x > float(SPACE_BETWEEN_POINTS_X) * float(i) + float(SPACE_BETWEEN_POINTS_X) )	{ continue; }
			if ( intersect_position.z < float(SPACE_BETWEEN_POINTS_Z) * float(j) )												{ continue; }
			if ( intersect_position.z > float(SPACE_BETWEEN_POINTS_Z) * float(j) + float(SPACE_BETWEEN_POINTS_Z) )	{ continue; }
			
			colliding_square.first = i;
			colliding_square.second = j;
			return true;
		}
	}

	return false;
}