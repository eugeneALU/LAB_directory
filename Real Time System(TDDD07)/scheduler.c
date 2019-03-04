/**
 * @file	scheduler.c
 * @author  Eriks Zaharans and Massimiiliano Raciti
 * @date    1 Jul 2013
 *
 * @section DESCRIPTION
 *
 * Cyclic executive scheduler library.
 */

/* -- Includes -- */
/* system libraries */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
/* project libraries */
#include "scheduler.h"
#include "task.h"
#include "timelib.h"

/* -- Defines -- */
/* Task DeadLine */
#define Deadline_MISSION			100
#define Deadline_NAVIGATE			100
#define Deadline_CONTROL			500
#define Deadline_REFINE				100
#define Deadline_REPORT				100
#define Deadline_COMMUNICATE		1000
#define Deadline_AVOID				100

/* -- Functions -- */

/**
 * Initialize cyclic executive scheduler
 * @param minor Minor cycle in miliseconds (ms)
 * @return Pointer to scheduler structure
 */
scheduler_t *scheduler_init(void)
{
	// Allocate memory for Scheduler structure
	scheduler_t *ces = (scheduler_t *) malloc(sizeof(scheduler_t));

	return ces;
}

/**
 * Deinitialize cyclic executive scheduler
 * @param ces Pointer to scheduler structure
 * @return Void
 */
void scheduler_destroy(scheduler_t *ces)
{
	// Free memory
	free(ces);
}

/**
 * Start scheduler
 * @param ces Pointer to scheduler structure
 * @return Void
 */
void scheduler_start(scheduler_t *ces)
{
	// Set timers
	timelib_timer_set(&ces->tv_started);
	timelib_timer_set(&ces->tv_cycle);
}

/**
 * Wait (sleep) till end of minor cycle
 * @param ces Pointer to scheduler structure
 * @return Void
 */
void scheduler_wait_for_timer(scheduler_t *ces)
{
	int sleep_time; // Sleep time in microseconds

	// Calculate time till end of the minor cycle
	sleep_time = (ces->minor * 1000) - (int)(timelib_timer_get(ces->tv_cycle) * 1000);

	// Add minor cycle period to timer
	timelib_timer_add_ms(&ces->tv_cycle, ces->minor);

	// Check for overrun and execute sleep only if there is no
	if(sleep_time > 0)
	{
		// Go to sleep (multipy with 1000 to get miliseconds)
		usleep(sleep_time);
	}
}

/**
 * Execute task
 * @param ces Pointer to scheduler structure
 * @param task_id Task ID
 * @return Void
 */
void scheduler_exec_task(scheduler_t *ces, int task_id)
{
	switch(task_id)
	{
	// Mission
	case s_TASK_MISSION_ID :
		task_mission();
		break;
	// Navigate
	case s_TASK_NAVIGATE_ID :
		task_navigate();
		break;
	// Control
	case s_TASK_CONTROL_ID :
		task_control();
		break;
	// Refine
	case s_TASK_REFINE_ID :
		task_refine();
		break;
	// Report
	case s_TASK_REPORT_ID :
		task_report();
		break;
	// Communicate
	case s_TASK_COMMUNICATE_ID :
		task_communicate();
		break;
	// Collision detection
	case s_TASK_AVOID_ID :
		task_avoid();
		break;
	// Other
	default :
		// Do nothing
		break;
	}
}

/**
 * Run scheduler
 * @param ces Pointer to scheduler structure
 * @return Void
 */
void scheduler_run(scheduler_t *ces)
{
	/* --- Local variables (define variables here) --- */
	struct timeval deadline, avoid_to_avoid, execute_time;
	double execute[8]={0.0}, times[8], A2A, cycle, T[8];
    double  min[8]={100.0,100.0,100.0,100.0,100.0,100.0,100.0,100.0};
	int over_deadline=0, over_A2A=0;
	int count=0;
	int i=0;

	/* --- Set minor cycle period --- */
	ces->minor =125;
	/* --- Write your code here --- */


    double time_computer;
    double waittime = 0;
    double sleep_time = 0;
    int sleep_time_int=1000;
    int sync =1;


    time_computer = timelib_unix_timestamp()/1000;
    waittime = ceil(time_computer);
    sleep_time = waittime - time_computer;
    sleep_time_int = (int)(sleep_time*1000000);

    if(sleep_time > 0)
    {
       usleep(sleep_time_int);      //usleep in micro seconds
    }

    scheduler_start(ces);
	while(count < 100){

		timelib_timer_set(&refine_to_send);		//for RFID read to send
		timelib_timer_set(&receive_to_control);	//for STOP command send to robot stop (ignore send time from desktop to robot computer)
		timelib_timer_set(&avoid_to_avoid);		//for Avoid interval
        timelib_timer_set(&deadline);			//for deadline measuring

        for (i=0; i<8;i++){
			timelib_timer_set(&execute_time);

			if (i==1) {
				timelib_timer_set(&execute_time);

				scheduler_exec_task(ces,s_TASK_COMMUNICATE_ID);

				T[s_TASK_COMMUNICATE_ID] = timelib_timer_get(execute_time);
                if (T[s_TASK_COMMUNICATE_ID] > execute[s_TASK_COMMUNICATE_ID]){
                    execute[s_TASK_COMMUNICATE_ID] =T[s_TASK_COMMUNICATE_ID];
                }
                if (T[s_TASK_COMMUNICATE_ID] < min[s_TASK_COMMUNICATE_ID] && T[s_TASK_COMMUNICATE_ID]!=0){
                        min[s_TASK_COMMUNICATE_ID] =T[s_TASK_COMMUNICATE_ID];
                }
				times[s_TASK_COMMUNICATE_ID]++;
			}

			if (i%2 == 0) {
			scheduler_exec_task(ces,s_TASK_NAVIGATE_ID);
			}
            T[s_TASK_NAVIGATE_ID] = timelib_timer_get(execute_time);
			if (T[s_TASK_NAVIGATE_ID] > execute[s_TASK_NAVIGATE_ID]){
                    execute[s_TASK_NAVIGATE_ID] =T[s_TASK_NAVIGATE_ID];
            }
            if (T[s_TASK_NAVIGATE_ID] < min[s_TASK_NAVIGATE_ID] && T[s_TASK_NAVIGATE_ID]!=0){
                    min[s_TASK_NAVIGATE_ID] = T[s_TASK_NAVIGATE_ID];
            }
			times[s_TASK_NAVIGATE_ID]++;


			if (i % 4 == 0) {       //if (i % 5 == 0) {
				timelib_timer_set(&execute_time);
				scheduler_exec_task(ces,s_TASK_CONTROL_ID);
                T[s_TASK_CONTROL_ID] = timelib_timer_get(execute_time);
                if (T[s_TASK_CONTROL_ID] > execute[s_TASK_CONTROL_ID]){
                    execute[s_TASK_CONTROL_ID] = T[s_TASK_CONTROL_ID];
                }
                if (T[s_TASK_CONTROL_ID] < min[s_TASK_CONTROL_ID] && T[s_TASK_CONTROL_ID]!=0){
                    min[s_TASK_CONTROL_ID] = T[s_TASK_CONTROL_ID];
                }
				times[s_TASK_CONTROL_ID]++;
			}

			A2A = timelib_timer_get(avoid_to_avoid);
			if (A2A > 150) {over_A2A++;}   //check timing (can't over 150ms)
			timelib_timer_set(&execute_time);
			scheduler_exec_task(ces,s_TASK_AVOID_ID);
            T[s_TASK_AVOID_ID] = timelib_timer_get(execute_time);
			if (T[s_TASK_AVOID_ID] > execute[s_TASK_AVOID_ID]){
                    execute[s_TASK_AVOID_ID] =T[s_TASK_AVOID_ID];
            }
            if (T[s_TASK_AVOID_ID] < min[s_TASK_AVOID_ID] && T[s_TASK_AVOID_ID]!=0){
                    min[s_TASK_AVOID_ID] = T[s_TASK_AVOID_ID];
            }
			timelib_timer_set(&avoid_to_avoid);
			times[s_TASK_AVOID_ID]++;

            timelib_timer_set(&execute_time);
			scheduler_exec_task(ces,s_TASK_REFINE_ID);
			T[s_TASK_REFINE_ID] = timelib_timer_get(execute_time);
			if (T[s_TASK_REFINE_ID] > execute[s_TASK_REFINE_ID]){
                    execute[s_TASK_REFINE_ID] =T[s_TASK_REFINE_ID];
            }
            if (T[s_TASK_REFINE_ID] < min[s_TASK_REFINE_ID] && T[s_TASK_REFINE_ID]!=0){
                    min[s_TASK_REFINE_ID] =T[s_TASK_REFINE_ID];
            }
			times[s_TASK_REFINE_ID]++;

			timelib_timer_set(&execute_time);
			scheduler_exec_task(ces,s_TASK_REPORT_ID);
			T[s_TASK_REPORT_ID] = timelib_timer_get(execute_time);
			if (T[s_TASK_REPORT_ID] > execute[s_TASK_REPORT_ID]){
                    execute[s_TASK_REPORT_ID] =T[s_TASK_REPORT_ID];
            }
            if (T[s_TASK_REPORT_ID] < min[s_TASK_REPORT_ID] && T[s_TASK_REPORT_ID]!=0){
                    min[s_TASK_REPORT_ID] =T[s_TASK_REPORT_ID];
            }
			times[s_TASK_REPORT_ID]++;

			timelib_timer_set(&execute_time);
			scheduler_exec_task(ces,s_TASK_MISSION_ID);
			T[s_TASK_MISSION_ID] = timelib_timer_get(execute_time);
			if (T[s_TASK_MISSION_ID] > execute[s_TASK_MISSION_ID]){
                    execute[s_TASK_MISSION_ID] =T[s_TASK_MISSION_ID];
            }
            if (T[s_TASK_MISSION_ID] < min[s_TASK_MISSION_ID] && T[s_TASK_MISSION_ID]!=0){
                    min[s_TASK_MISSION_ID] =T[s_TASK_MISSION_ID];
            }
			times[s_TASK_MISSION_ID]++;


			scheduler_wait_for_timer(ces);
			//printf("WAIT TIME: %f\n",timelib_timer_get(com_time));

        }
        count++;
        //printf("COUNT = %d\n",count);
    }
    //printf("Minor Cycle Time: %5f\n",execute[0]);
    printf("NAVIGATE MAX:         %5f\n",execute[s_TASK_NAVIGATE_ID]);
    printf("CONTROL MAX:          %5f\n",execute[s_TASK_CONTROL_ID]);
    printf("AVOID MAX:            %5f\n",execute[s_TASK_AVOID_ID]);
    printf("REFINE MAX:           %5f\n",execute[s_TASK_REFINE_ID]);
    printf("REPORT MAX:           %5f\n",execute[s_TASK_REPORT_ID]);
    printf("MISSION MAX:          %5f\n",execute[s_TASK_MISSION_ID]);
    printf("COMMUNICATION MAX:    %5f\n",execute[s_TASK_COMMUNICATE_ID]);
    printf("/////////////////////////////////////////////////////\n");
    printf("NAVIGATE MIN:         %5f\n",min[s_TASK_NAVIGATE_ID]);
    printf("CONTROL MIN:          %5f\n",min[s_TASK_CONTROL_ID]);
    printf("AVOID MIN:            %5f\n",min[s_TASK_AVOID_ID]);
    printf("REFINE MIN:           %5f\n",min[s_TASK_REFINE_ID]);
    printf("REPORT MIN:           %5f\n",min[s_TASK_REPORT_ID]);
    printf("MISSION MIN:          %5f\n",min[s_TASK_MISSION_ID]);
    printf("COMMUNICATION MIN:    %5f\n",min[s_TASK_COMMUNICATE_ID]);
	printf("/////////////////////////////////////////////////////\n");
	printf("DeadLine exceed Times: %5d\n", over_deadline);
	printf("/////////////////////////////////////////////////////\n");
	printf("Requirement:\n");
	printf("Avoid interval violate time:              %3d\n", over_A2A);
	printf("Average Victim_To_Send:                     %3f\n", victim_found_time/total_victim_found);
	printf("Victim_To_Send interval violate time:       %3d\n", over_R2S);
	printf("Average Receive_To_Control:               %3f\n", stopcmd_time/total_stopcmd_receive);
	printf("Receive_To_Control interval violate time: %3d\n", over_R2C);
	printf("/////////////////////////////////////////////////////\n");
	printf("Total send times:                 %5d\n", total_send);
	printf("Total times not receive go-ahead: %5d\n", not_receive_goahead);
	printf("Victim    send times: %5d\n", send_times[0]);
	printf("Location  send times: %5d\n", send_times[1]);
	printf("Pheromone send times: %5d\n", send_times[2]);
	printf("Stream    send times: %5d\n", send_times[3]);
    printf("Victim    omit times: %5d\n", discard_send[0]);
	printf("Location  omit times: %5d\n", discard_send[1]);
	printf("Pheromone omit times: %5d\n", discard_send[2]);
	printf("Stream    omit times: %5d\n", discard_send[3]);

}
