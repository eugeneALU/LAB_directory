/**
 * @file	taks.h
 * @author  Eriks Zaharans and Massimiiliano Raciti
 * @date	2 Jul 2013
 *
 * @section DESCRIPTION
 *
 * Header file for task that are run by scheduler.
 */

#ifndef __TASK_H
#define __TASK_H

/* -- Includes -- */
/* system libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* project libraries */
#include "config.h"
#include "def.h"
#include "enviroment.h"
#include "serialport.h"
#include "openinterface.h"
#include "rfid.h"
#include "robot.h"
#include "pf.h"
#include "queue.h"
#include "protocol.h"
#include "udp.h"
#include "pheromone.h"
#include "timelib.h"
#include "doublylinkedlist.h"
#include "debug.h"

/* -- Enumurations -- */

/* -- Types -- */

/**
 * @brief Task structure
 */
typedef struct s_TASK_STRUCT
{
	int enabled;

} task_t;

/**
 * @brief Task mission structure
 */
typedef struct s_TASK_MISSION_STRUCT
{
	// Victims
	int victim_count;
	victim_t victims[s_CONFIG_VICTIM_MAX_COUNT];

	// Stream
	long stream_counter;
	struct timeval stream_timer;

	// Go ahead
	struct timeval go_ahead_timer;

} task_mission_t;

/**
 * @brief Task control structure
 */
typedef struct s_TASK_CONTROL_STRUCT
{
	// OpenInterface request timer
	struct timeval request_timer;

} task_control_t;

/**
 * @brief Task pipe: refine -> report
 */
typedef struct s_TASKPIPE_REFINE_REPORT_STRUCT
{
	int event; // Boolean that notifies that victim id has been updated
	char victim_id[11]; // ID of tag 10 chars + '\0'

} taskpipe_refine_report_t;

/**
 * @brief Task pipe: report -> mission
 */
typedef struct s_TASKPIPE_REPORT_MISSION_STRUCT
{
	int event; // Boolean that notifies that victim structure has been updated
	victim_t victim; // Victim structure

} taskpipe_report_mission_t;

/**
 * @brief Task pipe: navigate -> control
 */
typedef struct s_TASKPIPE_NAVIGATE_CONTROL_STRUCT
{
	int event; // Boolean that notifies that next move has been updated
	int move; // Move

} taskpipe_navigate_control_t;



/* -- Constants -- */
#define s_TASK_EVENT_SET 				1
#define s_TASK_EVENT_RESET				0

/* Task IDs */
#define s_TASK_NOP_ID					0
#define s_TASK_MISSION_ID				1
#define s_TASK_NAVIGATE_ID				2
#define s_TASK_CONTROL_ID				3
#define s_TASK_REFINE_ID				4
#define s_TASK_REPORT_ID				5
#define s_TASK_COMMUNICATE_ID			6
#define s_TASK_AVOID_ID					7


/* -- Exported Variables -- */

/* -- Global Variables -- */
extern int over_R2S, over_R2C;
extern struct timeval refine_to_send, receive_to_control;
extern int total_victim_found, total_stopcmd_receive;
extern double victim_found_time, stopcmd_time;
extern int total_send;            // times that communication exceed time slot(some message missing)
extern int send_times[4], discard_send[4];        // how many message we send coresponding each type
extern int not_receive_goahead;
extern int flip;
extern int send_location;

/* System structures */
extern enviroment_t		*g_envs; // Enviroment definition
extern openinterface_t	*g_ois; // Open Interface
extern rfid_t			*g_rfids; // RFID Reader
extern pf_t				*g_pfs; // Particle filter
extern robot_t			*g_robot; // Estimated robot pose
extern pheromone_t		*g_phs; // Pheromones
extern udp_t			*g_udps; // UDP sockets connection

/* Data lists */
extern doublylinkedlist_t	*g_list_send; // Queue for data to be sent away (communication)
// more data lists for handling send priority
extern doublylinkedlist_t	*g_list_send_victim;        // in Report
extern doublylinkedlist_t	*g_list_send_location;      // in Control
extern doublylinkedlist_t	*g_list_send_pheromone;     // in Navigate
extern doublylinkedlist_t	*g_list_send_stream;      // in Mission

/* Data queues */
extern queue_t			*g_queue_mission; // Queue for data to mission task (commands, victims)
extern queue_t			*g_queue_navigate; // Queue for data to navigate task (pheromones)

/* Task structures */
extern task_t			g_task_mission; // Control for the mission task
extern task_t			g_task_navigate; // Control for the navigate task
extern task_t			g_task_control; // Control for the control task
extern task_t			g_task_refine; // Control for the refine task
extern task_t			g_task_report; // Control for the report task
extern task_t			g_task_communicate; // Control for the communicate task
extern task_t 			g_task_avoid; //Control for the collision detection task
extern task_t           g_task_gen_data_stream; // Control for the video streaming task


/* Specific task related structure */
extern task_mission_t	g_task_mission_data; // Structure that holds found victims and mission related things
extern task_control_t	g_task_control_data; // Structure that holds control task related variables

/* Task pipes */
extern taskpipe_refine_report_t			g_tp_refine_report; // refine -> report
extern taskpipe_report_mission_t		g_tp_report_mission; // report -> mission
extern taskpipe_navigate_control_t		g_tp_navigate_control; // navigate -> control

/* Communication information */
extern int g_message_sequence_id; // Massi thing
extern int g_tdma_slot; // Massi thing

extern int g_go_ahead;

/* -- Function Prototypes -- */
void task_init(int enable); // Initialize tasks
void task_destroy(void); // Deinitialize tasks

/* Tasks */
void task_mission(void); // Control mission
void task_navigate(void); // Control navigation
void task_control(void); // Sense, control, localization
void task_avoid(void); // Check the bump sensors for collision
void task_refine(void); // Refine position, localization
void task_report(void); // Report victim
void task_communicate(void); // Communication (receive and send data)

#endif /* __TASK_H */
