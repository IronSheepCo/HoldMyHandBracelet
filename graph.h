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
//1 - west
//2 - south
//3 - east
//4 - north
extern const uint8_t interest_zones_graph[][3];
extern const uint8_t interest_zones_length;

#define invert_dir(x) x<3?x+2:x-2

//1-forward
//2-right
//3-back
//4-left
uint8_t going_south(uint8_t cardinal_direction);
uint8_t going_east(uint8_t cardinal_direction);
uint8_t going_west(uint8_t cardinal_direction);
uint8_t going_north(uint8_t cardinal_direction); 

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

//returns the edge direction value between v_start and v_end
//this takes into account the order in which the 
//edge was entered in the graph
//in case there is no edge between the vertexes
//0 is returned
uint8_t find_edge( uint8_t v_start, uint8_t v_end);

//hash to hot spots
//first hot spot is used if the user is near the beacon
//second hot spot is used if the user if far from the beacon
extern const int hot_spots[][3];
extern const uint8_t hot_spots_length;

int16_t hotspot_to_peer_address( uint8_t hotspot );

#endif
