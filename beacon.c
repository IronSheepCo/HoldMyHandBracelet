#include "beacon.h"

const peer_coef peer_coefs[] = {
    {-12901, 5.389, (uint8_t)0},
    { 19316, 2.896, (uint8_t)0},
    { 31102, 3.24, (uint8_t)0},
    { 26586, 2.8962, (uint8_t)1},
    { 32690, 3.1102, (uint8_t)1},
    {-24576, 4.2606, (uint8_t)1},
    { 32217, 1.2763, (uint8_t)1}
};

const uint8_t peer_coefs_length = sizeof( peer_coefs )/sizeof( peer_coefs[0] );
