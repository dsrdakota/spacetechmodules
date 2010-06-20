/*
    gm_navigation
    By Spacetech
*/

#ifndef DEFINES_H
#define DEFINES_H

//#define FILEBUG

enum NavDirType
{
	NORTH = 0,
	SOUTH = 1,
	EAST = 2,
	WEST = 3,
	NORTHEAST = 4,
	NORTHWEST = 5,
	SOUTHEAST = 6,
	SOUTHWEST = 7
};

#define NUM_DIRECTIONS 4 // NORTH to WEST
#define NUM_DIRECTIONS_DIAGONAL 8 // NORTH to SOUTHWEST

// Sue me for thinking about the future and how SOUTHWESTNORTHEAST is a direction!
#define NUM_DIRECTIONS_MAX 8

#endif
