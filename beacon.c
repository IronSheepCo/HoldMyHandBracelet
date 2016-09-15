#include "beacon.h"

const peer_coef peer_coefs[] = {
    {-12901/*a9 */, 4.589, (uint8_t)5},
    { 19316/*a10*/, 2.896, (uint8_t)5},
    { 31102/*noa*/, 3.24, (uint8_t)1},
    { 26596/*a5 */, 2.8962, (uint8_t)1},
    { 32690/*a6 */, 3.1102, (uint8_t)5},
    {-24576/*a7 */, 3.0001, (uint8_t)5},
    { 32217/*a8 */, 1.2763, (uint8_t)1},
    { 6368/*a11*/ , 2.8072, (uint8_t)5}
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
