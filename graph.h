#ifndef __HOLDMYHAND_GRAPH_H__
#define __HOLDMYHAND_GRAPH_H__

#include <stdint.h>

/*
graph
1 - 2
1 - 4
1 - 5
2 - 3
3 - 4
5 - 6
6 - 7
6 - 8
6 - 9
9 - 10
9 - 11
11 - 12

1 - 13
13 - 14
*/

//array containing the graph for how the zones are connected
//third param is for direction, we need this because orientation is important
//1 - left
//2 - bottom
//3 - right
//4 - top
extern const uint8_t interest_zones_graph[][3];
extern const uint8_t interest_zones_length;

#define going_south(x) x<3?x+2:x-2 

//array containing definition of zones
//first term is the x position
//second term is the y position
//third term is the z position
//forth term is the size of it
//all measured in cm
extern const int interest_zones_def[][4];
extern const uint8_t interest_zones_def_length;

//finds the shortest route to an end location
//the returned array contains the parent for each
//node that's takes you to the next one
uint8_t* find_route( uint8_t end );

//hash to hot spots
//first hot spot is used if the user is near the beacon
//second hot spot is used if the user if far from the beacon
extern const int hot_spots[][3];
extern const uint8_t hot_spots_length;

#endif
