#ifndef __HOLDMYHAND_BEACON_H__
#define __HOLDMYHAND_BEACON_H__

#include <stdint.h>

typedef struct{
    int16_t peer_address;
    float   coef;
    //first byte is true if this should be use as an area indicator instead of a 
    //beacon for getting pin point location
    //second byte is true if the beacon has a far hot spot, 0 otherwise
    uint8_t is_area;
}peer_coef;

extern const peer_coef peer_coefs[];  
extern const uint8_t peer_coefs_length;
#endif
