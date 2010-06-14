/*
    gm_navigation
    By Spacetech
*/

#ifndef DEFINES_H
#define DEFINES_H

//#define FILEBUG
//#define DIAGONAL

enum NavDirType
{
	NORTH = 0,
	SOUTH = 1,
	EAST = 2,
	WEST = 3,

#ifdef DIAGONAL
	NORTHEAST = 5,
	NORTHWEST = 6,
	SOUTHEAST = 7,
	SOUTHWEST = 8,
#endif

	NUM_DIRECTIONS
};

#endif
