#include "beacon.h"

const peer_coef peer_coefs[] = {
    {-12901, 4.589, (uint8_t)5},
    { 19316, 2.896, (uint8_t)1},
    { 31102, 3.24, (uint8_t)1},
    { 26596, 2.8962, (uint8_t)1},
    { 32690, 3.1102, (uint8_t)1},
    {-24576, 3.0001, (uint8_t)1},
    { 32217, 1.2763, (uint8_t)1},
    { 6368 , 3.2072, (uint8_t)1}
};

const uint8_t peer_coefs_length = sizeof( peer_coefs )/sizeof( peer_coefs[0] );

uint8_t get_peer_index( int16_t peer_address )
{
 for( uint8_t i = 0; i<peer_coefs_length; i++ )
 {
  if( peer_coefs[i].peer_address == peer_address )
  {
    return i;
  }
 }

 return -1;
}
