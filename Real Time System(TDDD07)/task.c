/**
 * @file	task.c
 * @author  Eriks Zaharans and Massimiliano Raciti
 * @date    2 Jul 2013
 *
 * @section DESCRIPTION
 *
 * Contains task that are run by scheduler.
 */

/* -- Includes -- */
/* system libraries */

/* project libraries */
#include "task.h"

/* -- Defines -- */


/* -- Global Variables -- */
int over_R2S, over_R2C;
struct timeval refine_to_send, receive_to_control;
int total_victim_found, total_stopcmd_receive;
double victim_found_time, stopcmd_time;
int total_send;
int send_times[4] = {0,0,0,0}, discard_send[4] = {0,0,0,0};
int not_receive_goahead;
int flip;
int send_location;

/* System structures */
enviroment_t		*g_envs; // Enviroment definition
openinterface_t		*g_ois; // Open Interface
rfid_t				*g_rfids; // RFID Reader
pf_t				*g_pfs; // Particle filter
robot_t				*g_robot; // Estimated robot pose
pheromone_t			*g_phs; // Pheromones
udp_t				*g_udps; // UDP sockets connection

/* Data lists */
doublylinkedlist_t	*g_list_send; // Queue for data to be sent away (communication)
// more data lists for handling send priority
doublylinkedlist_t	*g_list_send_victim;
doublylinkedlist_t	*g_list_send_location;
doublylinkedlist_t	*g_list_send_pheromone;
doublylinkedlist_t	*g_list_send_stream;

/* Data queues */
queue_t				*g_queue_mission; // Queue for data to mission task (commands, victims)
queue_t				*g_queue_navigate; // Queue for data to navigate task (pheromones)

/* Task structures */
task_t			g_task_mission; // Control for the mission task
task_t			g_task_navigate; // Control for the navigate task
task_t			g_task_control; // Control for the control task
task_t			g_task_refine; // Control for the refine task
task_t			g_task_report; // Control for the report task
task_t			g_task_communicate; // Control for the communicate task
task_t 			g_task_avoid; //Control for the collision detection task

/* Specific task related structure */
task_mission_t	g_task_mission_data; // Structure that holds found victims and mission related things
task_control_t	g_task_control_data; // Structure that holds control task related variables

/* Task pipes */
taskpipe_refine_report_t		g_tp_refine_report; // refine -> report
taskpipe_report_mission_t		g_tp_report_mission; // report -> mission
taskpipe_navigate_control_t		g_tp_navigate_control; // navigate -> control

/* Communication information */
int g_message_sequence_id;
int g_tdma_slot;

int g_go_ahead;


/* -- Functions -- */

/**
 * Initialize tasks
 * @param enable If larger than 0, then enable all tasks at the start. Otherwise only Mission and Communicate tasks are enabled
 */
void task_init(int enable)
{
	// Init enviroment
	g_envs = enviroment_load(	g_config.enviroment_room_def_path,
								g_config.enviroment_tags_def_path);
	// Init open interface
	g_ois = openinterface_open(g_config.serialport_openinterface_port_path);
	// ----- THIS HERE IS HACK. Without this there is a problem with communication which prevents to read sensor data
	openinterface_sensors_update(g_ois, s_OI_SENSOR_PACKET_2, s_OI_SENSOR_PACKET_2_SIZE);
	openinterface_close(g_ois);
	g_ois = openinterface_open(g_config.serialport_openinterface_port_path);
	// -----

	// Init RFID
	g_rfids = rfid_open(g_config.serialport_rfid_port_path);
	// Init Particle filter
	g_pfs = pf_init(g_config.pf_particles_num,
					g_envs,
					g_config.pf_noise_move,
					g_config.pf_noise_turn,
					g_config.pf_noise_tag,
					g_config.pf_noise_wall,
					g_config.robot_radius);
	// Init Robot
	g_robot = robot_init(	g_config.robot_init_x,
							g_config.robot_init_y,
							g_config.robot_init_angle,
							0, 0, 0, 0,
							g_config.robot_radius);
	// Init Pheromones
	g_phs = pheromone_init(	g_envs,
							g_config.pheromone_width,
							g_config.pheromone_lifetime,
							g_config.pheromone_pheromone_radius,
							g_config.pheromone_eval_radius,
							g_config.pheromone_eval_dist,
							g_config.pheromone_sector_max_size);
	// Init UDP
	g_udps = udp_open(	g_config.udp_broadcast_ip,
						g_config.udp_port,
						g_config.udp_packet_size);


	// Init Queues
	// for communication
	g_list_send = doublylinkedlist_init();
	// Init more queues for communicate usage
    g_list_send_victim = doublylinkedlist_init();
    g_list_send_location = doublylinkedlist_init();
    g_list_send_pheromone = doublylinkedlist_init();
    g_list_send_stream = doublylinkedlist_init();


	// for mission
	g_queue_mission = queue_init();
	// for navigate
	g_queue_navigate = queue_init();


	// Init task
	if(enable > 0)
	{
		g_task_mission.enabled 			= s_TRUE;
		g_task_navigate.enabled 		= s_TRUE;
		g_task_control.enabled 			= s_TRUE;
		g_task_refine.enabled 			= s_TRUE;
		g_task_report.enabled 			= s_TRUE;
		g_task_communicate.enabled 		= s_TRUE;
		g_task_avoid.enabled 			= s_TRUE;
	}
	else
	{
		g_task_mission.enabled 			= s_TRUE;
		g_task_navigate.enabled 		= s_FALSE;
		g_task_control.enabled 			= s_FALSE;
		g_task_avoid.enabled 			= s_FALSE;
		g_task_refine.enabled 			= s_FALSE;
		g_task_report.enabled 			= s_FALSE;
		g_task_communicate.enabled 		= s_TRUE;
	}


	// Init mission data
	g_task_mission_data.victim_count = 0;
	// Init the data stream counter and timer
	g_task_mission_data.stream_counter = 0;
	timelib_timer_set(&g_task_mission_data.stream_timer);
	// Init "go ahead" timer
	timelib_timer_set(&g_task_mission_data.go_ahead_timer);

	// Init control data
	// Init openinterface request timer
	timelib_timer_set(&g_task_control_data.request_timer);

	//Init the message sequence id.
	g_message_sequence_id = 0;

	//TODO:load the slot from a config file
	g_tdma_slot=g_config.robot_id-1;

	g_go_ahead = 1;
}

/**
 * Deinitialize tasks
 */
void task_destroy(void)
{
	// Deinit enviroment
	enviroment_destroy(g_envs);
	// Deinit open interface
	openinterface_close(g_ois);
	// Deinit RFID
	rfid_close(g_rfids);
	// Deinit Particle filter
	pf_destroy(g_pfs);
	// Deinit robot
	robot_destroy(g_robot);
	// Deinit Pheromones
	pheromone_destroy(g_phs);
	// Deinit UDP
	udp_close(g_udps);

	// Deinit queues
	doublylinkedlist_destroy(g_list_send);

	doublylinkedlist_destroy(g_list_send_victim);
	doublylinkedlist_destroy(g_list_send_location);
	doublylinkedlist_destroy(g_list_send_pheromone);
	doublylinkedlist_destroy(g_list_send_stream);

	queue_destroy(g_queue_mission);
	queue_destroy(g_queue_navigate);
}
