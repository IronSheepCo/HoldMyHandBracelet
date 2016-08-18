#include "graph.h"
#include <string.h>
#include <stdio.h>

const uint8_t interest_zones_graph[][3] = {
 {1,  2,  1},
 {1,  3,  4},
 {1,  4,  2},
 {1,  5,  3},
 {5,  6,  4},
   };

const uint8_t interest_zones_length = sizeof(interest_zones_graph)/sizeof(interest_zones_graph[0]);

const int interest_zones_def[][4] = {
    {20  ,30  ,0  ,50  },
    {100 ,340 ,0  ,75  },
    {200 ,50  ,0  ,20  },
    {250 ,300 ,0  ,15  }
};

const uint8_t interest_zones_def_length = sizeof(interest_zones_def)/sizeof( interest_zones_def[0] );


const int hot_spots[][3] = {
    {  26596, 2, -1},
    {  32690, 4, -1},
    { -24576, 1, -1},
    {  31102, 3, -1}
};

const uint8_t hot_spots_length = sizeof(hot_spots)/sizeof( hot_spots[0] ); 

//this will hold the next node for the current node
//that the user needs to go to reach the destination
#define MAX_NODES 16
static uint8_t find_route_return[MAX_NODES];
static uint8_t find_route_visited_nodes[MAX_NODES];
static uint8_t find_route_current_distance[MAX_NODES]; 

uint8_t* find_route( uint8_t end )
{
 uint8_t index = 0, max = 1;
 uint8_t current_node = 0, nei;

 //clear the area
 memset( find_route_return, 0, sizeof(find_route_return) );

 //clear the visited array
 memset( find_route_visited_nodes, 0, sizeof(find_route_visited_nodes) ); 

 //clear the distance array to a big value
 memset( find_route_current_distance, 127, sizeof(find_route_current_distance) );

 find_route_visited_nodes[0] = end;
 find_route_current_distance[end] = 0;
 
 while( index<max )
 {
  current_node = find_route_visited_nodes[ index ];
 
  for( uint8_t i = 0; i < interest_zones_length; i++ )
  {
    nei = 0;

    if( interest_zones_graph[i][0] == current_node ) nei = interest_zones_graph[i][1];
    if( interest_zones_graph[i][1] == current_node ) nei = interest_zones_graph[i][0];
  
    //we have a match, search to see if the node was visited or not
    if( nei != 0 )
    {
        uint8_t found = 0;

        //check if the node was visited, if not, add it
        for( uint8_t j = 0; j < max; j++ )
        {
            //visited
            if( find_route_visited_nodes[j] == nei )
            {
                found = 1;
                break;
            }
        }
 
        if( found == 0 )
        {
            find_route_visited_nodes[max] = nei;
            max++;
        }

        //maybe we need to adjust the distance for nei
        found = find_route_current_distance[ current_node ] + 1;
        
        //better route found
        if( found < find_route_current_distance[nei] )
        {
            find_route_return[nei] = current_node;
            find_route_current_distance[nei] = found;
        } 
    }
  } 
  
  index++;
 }  

 return find_route_return;
}
