#ifndef __HOLDMYHAND_BEACON_H__
#define __HOLDMYHAND_BEACON_H__

#include <stdint.h>

typedef struct{
    int16_t peer_address;
    float   coef;
    //true if this should be use as an area indicator instead of a 
    //beacon for getting pin point location
    uint8_t is_area;
}peer_coef;

extern const peer_coef peer_coefs[];  

#endif
