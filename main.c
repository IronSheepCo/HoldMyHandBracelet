/*
 * Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

/** @example Board/nrf6310/s120/experimental/ble_app_multilink_central/main.c
 *
 * @brief Multilink BLE application main file.
 *
 * This file contains the source code for a sample central connecting to maximum 8 Peripherals.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "ble_gap.h"
#include "client_handling.h"
#include "softdevice_handler.h"
#include "app_util.h"
#include "app_error.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "pstorage.h"
#include "device_manager.h"
#include "app_trace.h"
#include "app_util.h"
#include "app_timer.h"
#include "nrf_delay.h"
#include "config.h"
#include "utils_math.h"
#include "graph.h"
#include "beacon.h"
#include "destination.h"
#include "SEGGER_RTT.h"

#ifdef BSP_BUTTON_1
#define BOND_DELETE_ALL_BUTTON_PIN       BSP_BUTTON_1                                   /**< Button used for deleting all bonded centrals during startup. */
#endif

#define APPL_LOG                   SEGGER_RTT_printf                                  /**< Debug logger macro that will be used in this file to do logging of debug information over UART. */

#define SEC_PARAM_BOND                   1                                              /**< Perform bonding. */
#define SEC_PARAM_MITM                   1                                              /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                           /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                              /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                              /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                             /**< Maximum encryption key size. */

#define SCAN_INTERVAL                    0x00A0                                         /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                      0x0050                                         /**< Determines scan window in units of 0.625 millisecond. */

#define MIN_CONNECTION_INTERVAL          MSEC_TO_UNITS(30, UNIT_1_25_MS)                /**< Determines minimum connection interval in millisecond. */
#define MAX_CONNECTION_INTERVAL          MSEC_TO_UNITS(60, UNIT_1_25_MS)                /**< Determines maximum connection interval in millisecond. */
#define SLAVE_LATENCY                    0                                              /**< Determines slave latency in counts of connection events. */
#define SUPERVISION_TIMEOUT              MSEC_TO_UNITS(4000, UNIT_10_MS)                /**< Determines supervision time-out in units of 10 millisecond. */

#define TARGET_DEV_NAME                  "Multilink"                                    /**< Target device name that application is looking for. */
#define MAX_PEER_COUNT                   DEVICE_MANAGER_MAX_CONNECTIONS                 /**< Maximum number of peer's application intends to manage. */

#define DEBUG_ALL   0

/**@brief Variable length data encapsulation in terms of length and pointer to data */
typedef struct
{
    uint8_t     * p_data;                                                      /**< Pointer to data. */
    uint16_t      data_len;                                                    /**< Length of data. */
}data_t;

static dm_application_instance_t          m_dm_app_id;                         /**< Application identifier. */
static uint8_t                            m_peer_count = 0;                    /**< Number of peer's connected. */

static bool                               m_memory_access_in_progress = false; /**< Flag to keep track of ongoing operations on persistent memory. */

static bool should_compute_position = false; /*this is set to true when a new interesting advertising packet is read*/

//array for next note to take to reach a certain point
//used for navigation
uint8_t*    route;

//current node index the user is in
//invalid value 255
uint8_t     current_node = 255;
uint16_t    current_smallest_distance = (1<<16)-1;

//a potential new node
//we only change the current node if there
//are 2 consecutive reading that show
//the same node
//invalid value 255
uint8_t     potential_new_node = 255;
//the number of consecutive times
//the new node was recorded
uint32_t     potential_new_node_count = 0;

//previous node index the user was in
//this is important so we can determine the user's orientation
//invalid value 255
uint8_t     previous_node = 255;

//next node previous recorded distance
//if the distance changes towards reaching the node
//we know the user is in the right direction
//so we can overide the directions to guide
//the user to go straight beacause
//he's approaching the next node
uint16_t    next_node_previous_distance = USHRT_MAX;

uint8_t     next_node_beacon_index = 255;

//the direction the user should take
//invalid value 255
uint8_t     current_direction = 255;

//this is the closest node
//without taking into account
//hotspot configuration
//invalid value 255
uint8_t     closest_node = 255;

//the id for the current orientation
//1-west, 2-south, 3-east, 4-north
//this could be set up front depending on the known entry
//point of a user
//it will get adjusted as the user moves around the environment
//but a good initial guess will be helpufull
uint8_t     current_orientation = 1;

//current beacon used for location the user
//invalid value 255
uint8_t     current_beacon_index = 255; 

//total number of ticks since the app started
uint32_t    total_ticks;

/**
 * @brief Scan parameters requested for scanning and connection.
 */
static const ble_gap_scan_params_t m_scan_param =
{
     1,                       // Active scanning not set.
     0,                       // Selective scanning not set.
     NULL,                    // White-list not set.
     (uint16_t)SCAN_INTERVAL, // Scan interval.
     (uint16_t)SCAN_WINDOW,   // Scan window.
     0                        // Never stop scanning unless explicitly asked to.
};

/**@brief Data storage for peer reading*/
typedef struct
{
    int16_t peer_address; /*hash of the peer address*/
    int8_t  measured_tx;  /*measured tx power at 1m*/
    int8_t  current_rssi;
    int8_t  prev_rssi[8];
    int8_t  rssi_start;
    int8_t  rssi_end;
    int8_t  rssi_count;
    int8_t  active;
    uint16_t current_distance;
}peer_info;

static void timeout_handler(void* p_context ){
}

static void app_init(){
    //print the defined zones
    for( uint8_t i = 0; i < interest_zones_def_length; i++)
    {
        SEGGER_RTT_printf(0, "zone def %d %d %d %d\n", interest_zones_def[i][0], interest_zones_def[i][1], interest_zones_def[i][2], interest_zones_def[i][3] );
    }

    //init the timer so we can get ticks later on
    app_timer_id_t timer_id;
    APP_TIMER_INIT( 0, 1, 1, NULL );
    app_timer_create( &timer_id, APP_TIMER_MODE_REPEATED, timeout_handler);
    app_timer_start( timer_id, APP_TIMER_TICKS(60*1000,0), NULL );
}

static peer_info peers[20];
static int peers_length = 0;

/** @breef Transforms a peer 6 bytes mac address to a hash*/
static int16_t peer_address_to_hash( uint8_t* peer_address )
{
    int16_t ret = 5381;
    
    for( int8_t i = 0; i<6; i++ )
    {
        ret = (ret*33+peer_address[ i ])%123534;
    }
   
    SEGGER_RTT_printf(0, "hash to address %d %d\n", *peer_address, ret);
 
    return ret;
}

/**@brief Adds or updates peer info
* The info is stored in the peers array
**/
static void add_peer_info( uint8_t* peer_address, int8_t rssi, int8_t tx )
{
    if( rssi == 0 )
    {
        return;
    }

    int16_t peer_hash = peer_address_to_hash( peer_address );

    for( int i = 0; i<peers_length; i++ )
    {
        if( peers[i].peer_address == peer_hash )
        {
            peers[i].current_rssi = rssi;
            
            //this is the same as doing a modulo, but faster
            peers[i].rssi_end = (peers[i].rssi_end + 1) & 7;
            peers[i].prev_rssi[ peers[i].rssi_end ] = rssi;
 
            //increase the rssi sample number
            peers[i].rssi_count ++;
            
            //if above 5 erase the last element
            if( peers[i].rssi_count >= NO_SAMPLES_RSSI )
            {
                //this is the same as doing a modulo, but faster
                peers[i].rssi_start = (peers[i].rssi_start + 1) & 7;
                peers[i].rssi_count --;
            }
            
            peers[ i ].active = MAX_ACTIVE_TICKS;
            
            return;
        }
    }

    //we need to add the peer
    peers[ peers_length].peer_address = peer_hash;
    peers[ peers_length].current_rssi = rssi;
    peers[ peers_length].prev_rssi[0] = rssi;
    peers[ peers_length].measured_tx = tx;
    peers[ peers_length].active = MAX_ACTIVE_TICKS;
    //circular buffer for rssi values
    peers[ peers_length].rssi_start = 0;
    peers[ peers_length].rssi_end = 1;
    peers[ peers_length].rssi_count = 1;
    peers[ peers_length].current_distance = 30000;

    SEGGER_RTT_printf(0,"Found new beacon hash: %d, tx_power: %d \n", peer_hash, tx);

    peers_length++;
}

/** @brief Decreases the active count for all available peers to a lower bound of 0 */
static void devaluate_peers()
{
    for( int i = 0; i<peers_length; i++ )
    {
        if( peers[i].active )
        {
            peers[i].active--;
        }
    }
}

/*@brief Compute distance based on rssi level*/
static float rssiToMeters( int8_t rssi, int8_t measured_tx, int16_t peer_address )
{
    float ret = 0;
    float coef = 2;

    //searching for beacon specific coef
    for( int8_t i = 0; i<peer_coefs_length; i++)
    {
        if( peer_coefs[i].peer_address == peer_address )
        {
            coef = peer_coefs[i].coef;
            break;
        }
    }

    if( DEBUG_ALL )
    {
        SEGGER_RTT_printf(0, "rssi: %d, tx: %d\n", rssi, measured_tx );
    }

    ret = pow(10, (measured_tx-rssi)/(10.0*coef) );

    return ret;
}

/**
@brief Return the n nearest beacons, this takes into account only the live
beacons
**/
static peer_info* find_nearest_beacons()
{
    return peers;
}

/**@brief Computes a current rssi based on a sample of existing rssi, this could be the average or the median 
   Returns -1 if there aren't enough entries from the beacon
*/
float working_rssi(uint8_t i)
{
    static int8_t values[NO_SAMPLES_RSSI], j;

    if( peers[i].rssi_start == peers[i].rssi_end )
    {
        return -1;
    }
 
    memset( values, 0, sizeof(values) ); 

    j = 0;
    for( uint8_t rssi_index = peers[i].rssi_start; rssi_index!=peers[i].rssi_end; rssi_index=(rssi_index+1)&7, j++ ){
        //avg_rssi += peers[i].prev_rssi[rssi_index];
        //copy values from the index
        values[j] = peers[i].prev_rssi[rssi_index];
    }

    //lets sort the values now
    bool found = false; 
    int8_t rssi_index = 0, tmp;
    do{
        found = false;
        for( rssi_index = 0; rssi_index<j-1; rssi_index++)
        {
            if( values[rssi_index]>values[rssi_index+1] )
            {
                tmp = values[rssi_index+1];
                values[rssi_index+1] = values[rssi_index];
                values[rssi_index] = tmp;
                found = true;
            }
        }
    }while(found);

    //avg_rssi /= peers[i].rssi_count;

    //if the final value is 0
    //means we have little data about
    //the current beacon, so we should
    //not use it's rssi value
    //this is usefull for not taking
    //into account newly discovered beacons
    if( values[j/2] == 0 )
    {
        return -1;
    }

    return values[j/2];
}

static uint8_t find_closest_hotspot_index()
{
    uint8_t ret = 255;
    int distance = 300000;
    
    //init values
    uint8_t nearest_node = 255;
    current_smallest_distance = USHRT_MAX;

    for( uint8_t i = 0; i < peers_length; i++ )
    {
        int hash = peers[i].peer_address;

        for( uint8_t j = 0; j<peer_coefs_length; j++ )
        {
            if( peer_coefs[j].peer_address == hash )
            {
                //not an hotspot beacon
                if( (peer_coefs[j].is_area & 1) == 0)
                {
                    break;
                }

                //very close beacon 
                //that's too far away
                if( (peer_coefs[j].is_area & 4)  && peers[i].current_distance>BEACON_NEAR_VALUE_SHORT )
                {
                    break;
                }

                //only near hotspot, but too far for near
                if( (peer_coefs[j].is_area & 2) == 0 && peers[i].current_distance>BEACON_NEAR_VALUE )
                {
                    //save the current beacon
                    //even if it's only a near beacon
                    if( peers[i].current_distance<current_smallest_distance)
                    {
                        nearest_node = i;
                        current_smallest_distance = peers[i].current_distance;
                    }

                    break;
                }

                if( peers[i].current_distance<distance )
                {
                    distance = peers[i].current_distance;
                    ret = i;
                }

                break;
            }
        }
    }

    //check if we have a near only beacon that's
    //closer than the far beacon
    if( current_smallest_distance < distance )
    {
        return nearest_node; 
    }
    
    return ret; 
}

static uint8_t is_beacon_near( uint8_t index )
{
    peer_info beacon = peers[index];

    if( beacon.current_distance<BEACON_NEAR_VALUE )
    {
        return 1;
    }

    return 0;
}

static void set_all_feedbacks()
{
    nrf_gpio_pin_set( LEFT_INDICATOR );
    nrf_gpio_pin_set( MID_INDICATOR );
    nrf_gpio_pin_set( RIGHT_INDICATOR );
}

static void stop_all_feedbacks()
{
    nrf_gpio_pin_clear( LEFT_INDICATOR );
    nrf_gpio_pin_clear( MID_INDICATOR );
    nrf_gpio_pin_clear( RIGHT_INDICATOR );
}

/** @brief Handles the feedback for the user based
on the current designated direction
this could be any of the following: print a debug message, light an LED, start a small vibrating motor
*/
static void handle_next_step()
{
    SEGGER_RTT_printf(0, "dir to take %d\n", current_direction);

    //clear all pins
    nrf_gpio_pin_clear( LEFT_INDICATOR );
    nrf_gpio_pin_clear( MID_INDICATOR );
    nrf_gpio_pin_clear( RIGHT_INDICATOR );

    //check if the user is approaching the next node
    //if so signal the user to go ahead
    if( SHOULD_USE_PREDICTIVE_DIRECTION && next_node_beacon_index != 255 && peers[ next_node_beacon_index ].current_distance < next_node_previous_distance - SIGNIFICANT_MOVEMENT )
    {
        nrf_gpio_pin_set( MID_INDICATOR );
        return;
    }

    //let's open up some leds
    switch( current_direction )
    {
        case 1:
            nrf_gpio_pin_set( MID_INDICATOR );
        break;
        case 2:
            nrf_gpio_pin_set( RIGHT_INDICATOR );
        break;
        case 3:
            nrf_gpio_pin_set( LEFT_INDICATOR );
            nrf_gpio_pin_set( RIGHT_INDICATOR );
        break;
        case 4:
            nrf_gpio_pin_set( LEFT_INDICATOR );
        break;
    }

    //we reached our destination
    //signal the user that we're finished
    if( current_node == final_destination_point )
    {
        SEGGER_RTT_WriteString(0, "we're here\n" );
        set_all_feedbacks();
    }
}

/** @brief Computes the next step to be taken
it takes into account current_node and route
when deciding what to do
*/
static void compute_next_step()
{
    //invalid value
    if( current_node == 255 )
    {
        //feedback for the user
        //it will use the previous set direction
        handle_next_step();
        return;
    }

    //getting the next node based on the current one
    //and the route previously computed 
    uint8_t next_node = route[ current_node ];

    //get the edge in the graph
    uint8_t dir = find_edge( current_node, next_node );

    //the next step to be taken is
    SEGGER_RTT_printf(0, "next hotspot %d\n", next_node );

    SEGGER_RTT_printf(0, "using orientation %d\n", current_orientation );   
 
    //we found a match
    if( dir )
    {
        //we need to take into account the current orienation
        //to determine the correct direction
        switch( current_orientation )
        {
            case 1: //west/left
                dir = going_west(dir);
            break;
            case 2: //south/down
                dir = going_south(dir);
            break;
            case 3: //east/right
                dir = going_east(dir);
            break;
            case 4: //north/up
                dir = going_north(dir);
            break;
            default:
            break;
        }
    }

    //we're in an unkown location
    //we need to recover
    //probably using the last known location
    //do nothing for now, we'll just use the previous one
    if( dir == 0 )
    {
    }
    else
    {
        current_direction = dir;
    }

    handle_next_step();
}

/**
@brief moves the user to a new node
this implies setting the new node
setting the old old node
computing the new orientation
*/
static void move_user_to_node( uint8_t node )
{
    uint32_t current_time;

    //only if already had a reading to move
    //to this node do it
    //otherwise set potential_new_node
    //and wait for a new node
    if( potential_new_node != node )
    {
        potential_new_node = node;

        //reset potential_new_node_count
        //by getting the current time
        app_timer_cnt_get( &potential_new_node_count);
        potential_new_node_count /= (APP_TIMER_CLOCK_FREQ/1000.0);

        return;
    }

    //we're in the same place
    if( node == current_node )
    {
        SEGGER_RTT_printf(0, "staying in node %d\n", node );
        return;
    }

    app_timer_cnt_get( &current_time );
    current_time /= (APP_TIMER_CLOCK_FREQ/1000.0);

    //check to see if there's an edge
    //between current node and next node
    //if there isn't then skip it
    //probably a bad reading
    if( current_node != 255 && find_edge( current_node, node ) == 0 )
    {
        //we do need to account for when
        //the user "jumps" from one place to another
        //so keep this value in a temp var
        //and if still gets good reading from that 
        //hotspot, move him next time
        if( current_time - potential_new_node_count < NON_EDGE_JUMP_MILLI )
        {
            return;
        }
    }

    //change the new node if we have enough consecutive
    //readings from the same node
    if( current_time - potential_new_node_count < CHANGE_NODE_MILLI )
    {
        return;
    }

    previous_node = current_node;
    
    //we'll do a jump now
    //trying to figure out a new orientation
    if( current_node != 255 && node != 255 && find_edge(current_node, node ) == 0 )
    {
        //parent graph for next node
        route = find_route( node );

        //look for parent of the future node
        //starting from current_node

        while( route[current_node] != node )
        {
            current_node = route[current_node];
        }

        current_orientation = find_edge( current_node, node );

        //put the route back
        route = find_route(final_destination_point);
    }

    current_node = node;

    app_timer_cnt_get( &potential_new_node_count );
    potential_new_node_count /= (APP_TIMER_CLOCK_FREQ/1000.0); 

    SEGGER_RTT_printf(0, "moving to node %d \n", node );

    //record the distance for the next node
    //we'll use this when
    int16_t peer_address = hotspot_to_peer_address( route[ current_node ] );

    if( peer_address != -1 )
    {
        next_node_beacon_index = get_peer_index(peer_address);
        
        if( next_node_beacon_index != -1 )
        {
            next_node_previous_distance = peers[ next_node_beacon_index ].current_distance;
        }
    }

    //try to find an edge in the graph with the previous and current
    //if none can be found, do nothing, we'll use the previous set
    //direction
    uint8_t new_dir = find_edge(previous_node, current_node);
    
    //we have a direction
    //set it as the current direction
    if( new_dir )
    {
        current_orientation = new_dir;
        SEGGER_RTT_printf(0, "new orientation %d\n", current_orientation );
    }
    else
    {
        SEGGER_RTT_printf(0, "did not found a new orientation\n" );
    }
}

/** @brief Computes the current position based on the connected peers */
static void compute_position()
{
    app_timer_cnt_get( &total_ticks );

    if( DEBUG_ALL )
    {
        SEGGER_RTT_WriteString(0,"computing distance \n"); 
    }

    //we always devaluate peers
    devaluate_peers();

    //no new information from the last poll
    //just skip
    if( should_compute_position == false ){
        return;
    }

    should_compute_position = false;

    for( uint8_t i = 0; i<peers_length; i++ )
    {
        if( peers[i].active <= 0 )
            continue;

        float avg_rssi = working_rssi(i);       

        //invalid value for rssi
        if( avg_rssi == -1 )
        {
            //use a very big distance
            peers[i].current_distance = 30000;
            continue;
        }

        float distance = rssiToMeters( (int)avg_rssi, peers[i].measured_tx, peers[i].peer_address );
        int int_distance = 100*distance; 

        if( SHOULD_USE_STICKINESS )
        {
            if( i == current_beacon_index )
            {
                int_distance -= STICK_TO_CURRENT_NODE_VALUE;
            }
            else
            {
                if( current_beacon_index != 255 )
                {
                    uint8_t v1 = hash_to_near(peers[i].peer_address);
                    uint8_t v2 = hash_to_near(peers[current_beacon_index].peer_address );
                    if( are_neighbours( v1, v2 ) )
                    {
                        int_distance -= STICK_NEIGHBOURS_DEDUCTION;
                    }
                }   
            }

            if( int_distance < BEACON_MIN_DISTANCE )
            {
                int_distance = BEACON_MIN_DISTANCE;
            }
        } 

        if( int_distance >= USHRT_MAX )
        {
            int_distance = USHRT_MAX;
        }       
 
        //if( DEBUG_ALL )
        {
            SEGGER_RTT_printf(0, "hash: %d distance: %d cm rssi:%d\n", peers[i].peer_address, int_distance, (int)avg_rssi);
        }

        peers[i].current_distance = int_distance;
    }

    //if using hotspots,
    //find the neareast hotspot
    if( SHOULD_USE_HOT_SPOTS )
    {
        uint8_t closest_hotspot_index = find_closest_hotspot_index();
        uint8_t near_number=0, far_number=0;

        if( closest_hotspot_index == 255 )
        {
            return;
        }

        SEGGER_RTT_printf(0, "using hotspot number %d with hash %d\n", closest_hotspot_index, peers[ closest_hotspot_index ].peer_address );

        for( uint8_t i = 0; i<hot_spots_length; i++ )
        {
            if( hot_spots[i][0] == peers[ closest_hotspot_index ].peer_address )
            {
                near_number = hot_spots[i][1];
                far_number  = hot_spots[i][2];

                //if there isn't a far number
                //use the hotspot for the near one
                //this could happen if the closest
                //beacon is a near only beacon
                if( far_number == 255 )
                {
                    far_number = near_number;
                }
            }
        }       

        current_beacon_index = closest_hotspot_index;
 
        //we need to decide if it's near of far
        if( is_beacon_near( closest_hotspot_index ) )
        {
            move_user_to_node( near_number );
            SEGGER_RTT_printf(0, "using the near hotspot %d\n", near_number);
        }
        else
        {
            move_user_to_node( far_number );
            SEGGER_RTT_printf(0, "using the far hotspot %d\n", far_number);
        }


        compute_next_step();
    }
    else
    {
    }
}

/**
 * @brief Connection parameters requested for connection.
 */
static const ble_gap_conn_params_t m_connection_param =
{
    (uint16_t)MIN_CONNECTION_INTERVAL,   // Minimum connection
    (uint16_t)MAX_CONNECTION_INTERVAL,   // Maximum connection
    0,                                   // Slave latency
    (uint16_t)SUPERVISION_TIMEOUT        // Supervision time-out
};

static void scan_start(void);

/**@brief Function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num        Line number of the failing ASSERT call.
 * @param[in] p_file_name     File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Callback handling device manager events.
 *
 * @details This function is called to notify the application of device manager events.
 *
 * @param[in]   p_handle      Device Manager Handle. For link related events, this parameter
 *                            identifies the peer.
 * @param[in]   p_event       Pointer to the device manager event.
 * @param[in]   event_status  Status of the event.
 */
static ret_code_t device_manager_event_handler(const dm_handle_t    * p_handle,
                                                 const dm_event_t     * p_event,
                                                 const ret_code_t     event_result)
{
    uint32_t       err_code;

    APPL_LOG(0,"event handler \n");

    switch(p_event->event_id)
    {
        case DM_EVT_CONNECTION:
            APPL_LOG(0,"[APPL]:[0x%02X] >> DM_EVT_CONNECTION\r\n", p_handle->connection_id);
//#ifdef ENABLE_DEBUG_LOG_SUPPORT
            ble_gap_addr_t * p_peer_addr;
            p_peer_addr = &p_event->event_param.p_gap_param->params.connected.peer_addr;
//#endif // ENABLE_DEBUG_LOG_SUPPORT
            APPL_LOG(0,"[APPL]:[%02X %02X %02X %02X %02X %02X]: Connection Established\r\n", p_peer_addr->addr[0], p_peer_addr->addr[1], p_peer_addr->addr[2], p_peer_addr->addr[3], p_peer_addr->addr[4], p_peer_addr->addr[5]);
            APPL_LOG(0,"\r\n");

            APPL_LOG(0,"[APPL]:[CI 0x%02X]: Requesting GATT client create\r\n",
                     p_handle->connection_id);
            err_code = client_handling_create(p_handle, p_event->event_param.p_gap_param->conn_handle);
            APP_ERROR_CHECK(err_code);

            m_peer_count++;
            if (m_peer_count < MAX_PEER_COUNT)
            {
                scan_start();
            }
            APPL_LOG(0,"[APPL]:[0x%02X] << DM_EVT_CONNECTION\r\n", p_handle->connection_id);
            break;
        case DM_EVT_DISCONNECTION:
            APPL_LOG(0,"[APPL]:[0x%02X] >> DM_EVT_DISCONNECTION\r\n", p_handle->connection_id);

            err_code = client_handling_destroy(p_handle);
            APP_ERROR_CHECK(err_code);

            if (m_peer_count == MAX_PEER_COUNT)
            {
                scan_start();
            }
            m_peer_count--;
            APPL_LOG(0,"[APPL]:[0x%02X] << DM_EVT_DISCONNECTION\r\n", p_handle->connection_id);
            break;
        case DM_EVT_SECURITY_SETUP:
        {
            dm_handle_t handle = (*p_handle);
            APPL_LOG(0,"[APPL]:[0x%02X] >> DM_EVT_SECURITY_SETUP\r\n", p_handle->connection_id);
            // Slave securtiy request received from peer, if from a non bonded device, 
            // initiate security setup, else, wait for encryption to complete.
            err_code = dm_security_setup_req(&handle);
            APP_ERROR_CHECK(err_code);
            APPL_LOG(0,"[APPL]:[0x%02X] << DM_EVT_SECURITY_SETUP\r\n", p_handle->connection_id);
            break;
        }
        case DM_EVT_SECURITY_SETUP_COMPLETE:
            APPL_LOG(0,"[APPL]:[0x%02X] >> DM_EVT_SECURITY_SETUP_COMPLETE, result 0x%08X\r\n",
                      p_handle->connection_id, event_result);
            APPL_LOG(0,"[APPL]:[0x%02X] << DM_EVT_SECURITY_SETUP_COMPLETE\r\n",
                      p_handle->connection_id);
            break;
        case DM_EVT_LINK_SECURED:
            APPL_LOG(0,"[APPL]:[0x%02X] >> DM_LINK_SECURED_IND, result 0x%08X\r\n",
                      p_handle->connection_id, event_result);
            APPL_LOG(0,"[APPL]:[0x%02X] << DM_LINK_SECURED_IND\r\n", p_handle->connection_id);
            break;
        case DM_EVT_DEVICE_CONTEXT_LOADED:
            APPL_LOG(0,"[APPL]:[0x%02X] >> DM_EVT_LINK_SECURED\r\n", p_handle->connection_id);
            APP_ERROR_CHECK(event_result);
            APPL_LOG(0,"[APPL]:[0x%02X] << DM_EVT_DEVICE_CONTEXT_LOADED\r\n", p_handle->connection_id);
            break;
        case DM_EVT_DEVICE_CONTEXT_STORED:
            APPL_LOG(0,"[APPL]:[0x%02X] >> DM_EVT_DEVICE_CONTEXT_STORED\r\n", p_handle->connection_id);
            APP_ERROR_CHECK(event_result);
            APPL_LOG(0,"[APPL]:[0x%02X] << DM_EVT_DEVICE_CONTEXT_STORED\r\n", p_handle->connection_id);
            break;
        case DM_EVT_DEVICE_CONTEXT_DELETED:
            APPL_LOG(0,"[APPL]:[0x%02X] >> DM_EVT_DEVICE_CONTEXT_DELETED\r\n", p_handle->connection_id);
            APP_ERROR_CHECK(event_result);
            APPL_LOG(0,"[APPL]:[0x%02X] << DM_EVT_DEVICE_CONTEXT_DELETED\r\n", p_handle->connection_id);
            break;
        default:
            break;
    }

    // Relay the event to client handling module.
    err_code = client_handling_dm_event_handler(p_handle, p_event, event_result);
    APP_ERROR_CHECK(err_code);

    return NRF_SUCCESS;
}

/**
 * @brief Parses advertisement data, providing length and location of the field in case
 *        matching data is found.
 *
 * @param[in]  Type of data to be looked for in advertisement data.
 * @param[in]  Advertisement report length and pointer to report.
 * @param[out] If data type requested is found in the data report, type data length and
 *             pointer to data will be populated here.
 *
 * @retval NRF_SUCCESS if the data type is found in the report.
 * @retval NRF_ERROR_NOT_FOUND if the data type could not be found.
 */
static uint32_t adv_report_parse(uint8_t type, data_t * p_advdata, data_t * p_typedata)
{
    uint32_t index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->data_len)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type   = p_data[index+1];

        if (field_type == type)
        {
            p_typedata->p_data   = &p_data[index+2];
            p_typedata->data_len = field_length-1;
            return NRF_SUCCESS;
        }
        index += field_length+1;
    }
    return NRF_ERROR_NOT_FOUND;
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t        err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_ADV_REPORT:
        {
            data_t adv_data;
            data_t type_data;

            memset(&type_data, 0, sizeof(type_data) );

            // Initialize advertisement report for parsing.
            adv_data.p_data = p_ble_evt->evt.gap_evt.params.adv_report.data;
            adv_data.data_len = p_ble_evt->evt.gap_evt.params.adv_report.dlen;

            err_code = adv_report_parse(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
                                      &adv_data,
                                      &type_data);

            if (err_code != NRF_SUCCESS)
            {
                // Compare short local name in case complete name does not match.
                err_code = adv_report_parse(BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME,
                                          &adv_data,
                                          &type_data);
            }
            else
            {
                //APPL_LOG(0, "name %s\n",type_data.p_data);
            }

            //getting manufacturer specific data
            err_code = adv_report_parse(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
                                        &adv_data,
                                        &type_data);

            if (err_code == NRF_SUCCESS)
            {
               //lat = ((float*)type_data.p_data)[0];
               //lon = ((float*)type_data.p_data)[1];

               //ignore beacons that we don't know
               //we use the manufacturer data for this
               if( type_data.p_data[0] != 0x59 ){

                if( DEBUG_ALL )
                {
                    SEGGER_RTT_WriteString(0, "returning \n");
                }

                return; 
               }

                //lat = 0;//type_data.p_data[3];
                //lon = 0;//type_data.p_data[7];

                char container[128];
                memset( container, 0, sizeof(container) );

                //SEGGER_RTT_WriteString(0, container);

                int8_t rssi = p_ble_evt->evt.gap_evt.params.adv_report.rssi;
                //SEGGER_RTT_printf(0, "rssi level %d\n", rssi);

                //reading the measured power level at 1 m
                int8_t power_level;
                memcpy( &power_level, type_data.p_data+10, sizeof(int8_t) );
                
                //SEGGER_RTT_printf( 0, "power level %d\n", power_level );            
    
                uint8_t peer_address[6];
                memcpy( peer_address, p_ble_evt->evt.gap_evt.params.adv_report.peer_addr.addr, sizeof( uint8_t ) * 6);
                
                //SEGGER_RTT_WriteString(0, "peer address: ");

                //add peer address, this will be used
                //at every tick to determine the 
                //location of the bracelet
                add_peer_info( peer_address, rssi, power_level ); 

                err_code = adv_report_parse(BLE_GAP_AD_TYPE_TX_POWER_LEVEL,
                                            &adv_data,
                                            &type_data);

                if( err_code == NRF_SUCCESS )
                {
                    //SEGGER_RTT_printf(0, "tx power %x\n", type_data.p_data);
                }

                //let the poll service recompute the position
                //we have new data
                should_compute_position = true;
            }

            // Verify if short or complete name matches target.
            if ((err_code == NRF_SUCCESS) &&
               (0 == memcmp(TARGET_DEV_NAME,type_data.p_data,type_data.data_len)))
            {
                //err_code = sd_ble_gap_scan_stop();
                //if (err_code != NRF_SUCCESS)
                //{
                //    APPL_LOG(0,"[APPL]: Scan stop failed, reason %d\r\n", err_code);
                //}

                //err_code = sd_ble_gap_connect(&p_ble_evt->evt.gap_evt.params.adv_report.
                  //                            peer_addr,
                  //                            &m_scan_param,
                    //                          &m_connection_param);

                //APPL_LOG(0, "sd_ble_gap_connect code %d\n", err_code );

               // if (err_code != NRF_SUCCESS)
                //{
                  //  APPL_LOG(0,"[APPL]: Connection Request Failed, reason %d\r\n", err_code);
                //}
            }
            break;
        }
        case BLE_GAP_EVT_TIMEOUT:
            if(p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)
            {
                APPL_LOG(0,"[APPL]: Scan Timedout.\r\n");
            }
            else if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                APPL_LOG(0,"[APPL]: Connection Request Timedout.\r\n");
            }
            break;
        default:
            break;
    }
}


/**@brief Function for handling the Application's system events.
 *
 * @param[in]   sys_evt   system event.
 */
static void on_sys_evt(uint32_t sys_evt)
{
    switch(sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
        case NRF_EVT_FLASH_OPERATION_ERROR:
            if (m_memory_access_in_progress)
            {
                m_memory_access_in_progress = false;
                scan_start();
            }
            break;
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack event has
 *  been received.
 *
 * @param[in] p_ble_evt     Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    dm_ble_evt_handler(p_ble_evt);
    client_handling_ble_evt_handler(p_ble_evt);
    on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    APPL_LOG(0,"ble stack init\n");

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

    // Enable BLE stack.
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
#ifdef S130
    APPL_LOG(0,"running on s130 stack \n");
    ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
    ble_enable_params.gatts_enable_params.service_changed = false;
#ifdef S120
    APPL_LOG(0,"running on s120 stack \n");
    ble_enable_params.gap_enable_params.role              = BLE_GAP_ROLE_CENTRAL;
#endif

    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for System events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Device Manager.
 *
 * @details Device manager is initialized here.
 */
static void device_manager_init(void)
{
    dm_application_param_t param;
    dm_init_param_t        init_param;

    uint32_t err_code;

    APPL_LOG(0,"device manager init \n");

    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    init_param.clear_persistent_data = false;

#ifdef BOND_DELETE_ALL_BUTTON_PIN
    // Clear all bonded devices if user requests to.
    init_param.clear_persistent_data =
        ((nrf_gpio_pin_read(BOND_DELETE_ALL_BUTTON_PIN) == 0)? true: false);
#endif

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&param.sec_param, 0, sizeof (ble_gap_sec_params_t));

    // Event handler to be registered with the module.
    param.evt_handler            = device_manager_event_handler;

    // Service or protocol context for device manager to load, store and apply on behalf of application.
    // Here set to client as application is a GATT client.
    param.service_type           = DM_PROTOCOL_CNTXT_GATT_CLI_ID;

    // Secuirty parameters to be used for security procedures.
    param.sec_param.bond         = SEC_PARAM_BOND;
    param.sec_param.mitm         = SEC_PARAM_MITM;
    param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    param.sec_param.oob          = SEC_PARAM_OOB;
    param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    param.sec_param.kdist_periph.enc = 1;
    param.sec_param.kdist_periph.id  = 1;

    err_code = dm_register(&m_dm_app_id,&param);
    APP_ERROR_CHECK(err_code);
    APPL_LOG(0,"error code for dm_register %d\n",err_code);
}


/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
#ifdef BOND_DELETE_ALL_BUTTON_PIN
  	// Set Wakeup and Bonds Delete buttons as wakeup sources.
    //nrf_gpio_cfg_sense_input(BOND_DELETE_ALL_BUTTON_PIN,
                            // BUTTON_PULL,
                             //NRF_GPIO_PIN_SENSE_LOW);
#endif
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

/**@breif Function to start scanning.
 */
static void scan_start(void)
{
    uint32_t err_code;
    uint32_t count;
    // Verify if there is any flash access pending, if yes delay starting scanning until 
    // it's complete.
    err_code = pstorage_access_status_get(&count);
    APP_ERROR_CHECK(err_code);
    
    if (count != 0)
    {
        m_memory_access_in_progress = true;
        return;
    }

    APPL_LOG(0, "started scanning\n");

    err_code = sd_ble_gap_scan_start(&m_scan_param);
    APPL_LOG(0, "start scan error code %d\n", err_code);
    APP_ERROR_CHECK(err_code);
}

static void init_pins()
{
    nrf_gpio_cfg_output( LEFT_INDICATOR );
    nrf_gpio_cfg_output( MID_INDICATOR );
    nrf_gpio_cfg_output( RIGHT_INDICATOR );
}

/**@brief Function for application main entry.
 */
int main(void)
{
    // Initialization of various modules.
    app_trace_init();
    //LEDS_CONFIGURE(LEDS_MASK);
    //LEDS_OFF(LEDS_MASK);
    buttons_init();
    init_pins();

    for (uint16_t counter=0; counter<2500; counter+=300)
    {
     set_all_feedbacks();
    
     nrf_delay_ms(150);

     stop_all_feedbacks();

     nrf_delay_ms(150);
    }

    route = find_route(final_destination_point); 

    ble_stack_init();
    client_handling_init();
    device_manager_init();
    app_init();

    // Start scanning for devices.
    scan_start();

    for (;;)
    {
        power_manage();
        compute_position();
    }

}
