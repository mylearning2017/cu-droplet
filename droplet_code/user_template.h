#include "droplet_init.h"

#define RED_THRESH 80
#define GREEN_THRESH 60

#define MOVE_DIST_SCALAR            50
#define WALKAWAY_STEPS              1000

#define START_DELAY_TIME            100     // in ms
#define COLLABORATE_DURATION        3000    // in ms

#define GROUP_MEMBERSHIP_TIMEOUT    2000    // in ms
#define HEART_RATE                  1000    // in ms

#define BUILD_NEIGHBOR_LIST_TIME	5000   // maybe we do not need this one anymore

typedef struct GROUP_LIST_ITEM
{
	uint16_t ID;
	uint16_t ms_age;
	struct GROUP_LIST_ITEM* next;
	struct GROUP_LIST_ITEM* prev;
} group_item;
group_item* group_root;

typedef enum
{
	INIT,
	FINAL,
	IDLE,
	FRONTIER,
	LIGHT_ON,
	WAIT
} State;
State	state;

uint16_t who_asked_me = 0; // stores the one who put a query
char* msg_f; // F! = Found!
char* msg_q; // F? = Found?
char* msg_h; // a message for signalling its presence to its neighbors

int8_t redSenseVal, greenSenseVal;
uint16_t	current_group_size;
uint16_t	yes_count;
uint32_t	start_delay_time;
uint32_t	heartbeat_time;
uint32_t	voting_time;
uint32_t	collab_time;
uint8_t		last_move_dir;
uint8_t		is_end;
uint8_t     num_sent;

// Task Allocation Helper Functions
uint16_t	update_group_size	( uint16_t	time_to_add	);
void		add_group_member	( uint16_t	senderID	);

void        send_query			();
void        make_list_of_neighbors ();
void        idle_waiting		();
void        wait_waiting		();

void		change_state		( State		new_state	);

void init();
void loop();
void handle_msg(ir_msg* msg_struct);

