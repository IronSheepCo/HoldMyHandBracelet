#define SHOULD_USE_HOT_SPOTS             1                                              /**< Flag that enables the use of hotspots. If it's enabled then the exact position is not computed, but the nearest beacon is used and depending on the distance from the hot spot (near, far) a hot spot is selected */

#define SHOULD_USE_PREDICTIVE_DIRECTION     0 /**<Flag that enables to adjust the direction based on getting closer to the next beacon. If the user gets closer to the next POI then the direction should be forward and not the direction given by the graph and current orientation*/

#define BEACON_NEAR_VALUE 350 /**number of cm under which we consider to be near a beacon*/
#define BEACON_NEAR_VALUE_SHORT 275 /**number of cm under which we consider a short near beacon*/

#define LEFT_INDICATOR 24
#define MID_INDICATOR 28
#define RIGHT_INDICATOR 30

#define NON_EDGE_JUMP_MILLI 1500 /**how many milliseconds a user needs to be inside a non edged node before moving the user BY FORCE to that node*/
#define CHANGE_NODE_MILLI 500 /**how many milliseconds a user needs to be inside a node before moving to that node*/

//number of cm that mean that a movement
//is to be counted as important
//this is primarilly used to decide
//that a user is approaching  
#define     SIGNIFICANT_MOVEMENT    75 

#define NO_SAMPLES_RSSI 7

/**How many tick does a peer get when he's around
* each tick we subtract one and when active reaches 0 we know the peer is out of
* range
*/
#define MAX_ACTIVE_TICKS 20

#define SHOULD_USE_STICKINESS 1 /** Flag for enabling/disabling the use of stickiness*/

#define STICK_TO_CURRENT_NODE_VALUE 50 /** Value used to be subtracted from current beacon when moving from one place to another  */
