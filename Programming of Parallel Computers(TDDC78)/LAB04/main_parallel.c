#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <mpi.h>

#include "coordinate.h"
#include "definitions.h"
#include "physics.h"
#include "create_datatype.h"

typedef struct list
{
	pcord_t particle;
	bool collisions;
	struct list *next;
} particle_list;

float rand1()
{
	return (float)(rand() / (float)RAND_MAX);
}

void init_collisions(particle_list* head)
{
	particle_list *l;
	l = head;
	while(l!= NULL){
		l->collisions = 0;
		l=l->next;
	}
	return;
}

int main(int argc, char **argv)
{

	unsigned int time_stamp = 0, time_max;
	struct timespec stime, etime;

	// parse arguments
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s simulation_time\n", argv[0]);
		fprintf(stderr, "For example: %s 10\n", argv[0]);
		exit(1);
	}

	time_max = atoi(argv[1]);

	/* Initialize */
	// 1. set the walls
	cord_t wall;
	wall.y0 = wall.x0 = 0;
	wall.x1 = BOX_HORIZ_SIZE;
	wall.y1 = BOX_VERT_SIZE;

	// 2. allocate particle buffer and initialize the particles
	pcord_t *particles = (pcord_t *)malloc(INIT_NO_PARTICLES * sizeof(pcord_t));

	/* MPI init */
	int n_task;
	int dim[2] = {0,1};	 //2D Matrix specified second dimension 
	int period[2] = {0};     //without periodic 
	int reorder = 1; 	 //reorder
	MPI_Comm CART_COMM;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &n_task);	    //get total nuber of processor
	MPI_Dims_create(n_task, 2, dim);	    //create proper 2 dimension
	MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &CART_COMM);	//create 2D topology

	/* obtain neighbor rank */
	int mycord[2] = {0};
	int nbcord[2] = {0};
	int myrank;// oldrank;
	int uprank, downrank;

	MPI_Cart_get(CART_COMM, 2, dim, period, mycord);	//get personal coordinate 
	MPI_Cart_rank(CART_COMM, mycord, &myrank);		//get personal rank in new CART_COMM according to coordinate 
	
	nbcord[1]=mycord[1];
	nbcord[0]=mycord[0]+1;
	if (nbcord[0] < n_task){
		MPI_Cart_rank(CART_COMM, nbcord, &downrank);
	}
	else {downrank = -1;}
	
	nbcord[0]=mycord[0]-1;
	if (nbcord[0] > -1){
		MPI_Cart_rank(CART_COMM, nbcord, &uprank);
	}
	else {uprank = -1;}
	
	/* get virtual boundry */ 
	int chunk = BOX_VERT_SIZE / n_task;
	int start = mycord[0]*chunk;
	int end = (mycord[0]+1)*chunk - 1;
	if (mycord[0] == n_task-1) {
		end = BOX_VERT_SIZE;
	}

	/* initialize particles */
	srand(time(NULL) + 1234 + myrank);	// +myrank to prevent the same pattern generate in different thread because of the pseudo random
	float r, a;
	for (int i = 0; i < INIT_NO_PARTICLES; i++)
	{
		// initialize random position
		particles[i].x = wall.x0 + rand1() * BOX_HORIZ_SIZE;
		particles[i].y = start + rand1() * chunk;	//limit y value is inside the boundry

		// initialize random velocity
		r = rand1() * MAX_INITIAL_VELOCITY;
		a = rand1() * 2 * PI;
		particles[i].vx = r * cos(a);
		particles[i].vy = r * sin(a);
	}
		
	particle_list *head = (particle_list *)malloc(sizeof(particle_list));
	head->next = (particle_list *)malloc(sizeof(particle_list));
	head->particle = particles[0];
	particle_list *l = head->next;

	for (int i=1; i<INIT_NO_PARTICLES; i++)
	{	
		l->particle = particles[i];
		if (i<INIT_NO_PARTICLES-1){
			l->next = (particle_list *)malloc(sizeof(particle_list));
		}
		else {
			l->next = NULL;			
		}		
		l = l->next;		
	}
	free(particles);

	/* create data_type */
	MPI_Datatype MPI_PARTICLE; 
	create_datatype_particle(&particles[0], &MPI_PARTICLE);

	/* buffer and variables for sending data */
	'''
		the size of buffer is predefined to a resonable size
		better using C++ vector to dynamic adjust the size
	'''
	pcord_t receiveUP_buffer[MAX_NO_PARTICLES/5];  
	pcord_t receiveDN_buffer[MAX_NO_PARTICLES/5];
	pcord_t sendUP_buffer[MAX_NO_PARTICLES/5];
	pcord_t sendDN_buffer[MAX_NO_PARTICLES/5];	
	particle_list *tmp, *k;
	int outlierUP, outlierDN;
	int collision=0, collision_buf=0;
	float pressure = 0, pressure_buf = 0;

	clock_gettime(CLOCK_REALTIME, &stime);

	/* Main loop */
	for (time_stamp = 0; time_stamp < time_max; time_stamp++)
	{ 	
		/* get outlier */
		outlierUP = 0;
		outlierDN = 0;

		l=head;
		while(l!= NULL)
		{				
			if (l->particle.y < start && uprank != -1){
				sendUP_buffer[outlierUP] = l->particle;				
				outlierUP++;
				if (l==head){
					head = l->next; //handle head pointer
				}
				else{
					tmp->next = l->next;
				}
				free(l);			
			}	
			
			else if(l->particle.y > end && downrank != -1){
				sendDN_buffer[outlierDN] = l->particle;
				outlierDN++;
				if (l==head){
					head = l->next; //handle head pointer
				}
				else{
					tmp->next = l->next;
				}
				free(l);			
			}
			else{
				tmp = l;
			}
			l = l->next;
		}

		/* send outlier */
		if (uprank != -1) {
			MPI_Send(sendUP_buffer, outlierUP, MPI_PARTICLE, uprank, 0, MPI_COMM_WORLD);
			//printf("time{%d} process(%d): send:%d to [%d]\n",time_stamp,myrank, outlierUP,uprank);
		}
	
		if (downrank != -1) {
			MPI_Send(sendDN_buffer, outlierDN, MPI_PARTICLE, downrank, 0, MPI_COMM_WORLD);	
			//printf("time{%d} process(%d): send:%d to [%d]\n",time_stamp,myrank, outlierDN,downrank);
		}
		
		MPI_Status receiveUP_status,receiveDN_status;
		int up_num, down_num;		
		
		if (uprank != -1) {
			MPI_Probe(uprank,0,MPI_COMM_WORLD,&receiveUP_status);
			MPI_Get_count(&receiveUP_status, MPI_PARTICLE, &up_num);
			MPI_Recv(receiveUP_buffer, up_num, MPI_PARTICLE, uprank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("time{%d} process(%d): receive:%d from [%d]\n",time_stamp,myrank, up_num,uprank);
		}

		if (downrank != -1) {
			MPI_Probe(downrank,0,MPI_COMM_WORLD,&receiveDN_status);
			MPI_Get_count(&receiveDN_status, MPI_PARTICLE, &down_num);
			MPI_Recv(receiveDN_buffer, down_num, MPI_PARTICLE, downrank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("time{%d} process(%d): receive:%d from [%d]\n",time_stamp,myrank, down_num,downrank);
		}

		/* add receive particles into list */
		int i;
		for(i=0 ; i < up_num ; i++){
			tmp = head; 
			head = (particle_list *)malloc(sizeof(particle_list)); 
			head->next = tmp;
			head->particle = receiveUP_buffer[i];
		}
		for(i=0 ; i < down_num ; i++){
			tmp = head; 
			head = (particle_list *)malloc(sizeof(particle_list)); 
			head->next = tmp;
			head->particle = receiveDN_buffer[i];
		}

		// for each time stamp
		init_collisions(head);
		
		l=head;
		while(l!= NULL) // for all particles
		{
			if (l->collisions){
				l = l->next;
				continue;
			}

			/* check for collisions */
			k = l->next;
			while(k!= NULL)
			{	
				if (k->collisions){
					k = k->next;
					continue;
				}
				float t = collide(&(l->particle), &(k->particle));
				if (t != -1) // collision
				{
					l->collisions = k->collisions = 1;
					interact(&(l->particle), &(k->particle), t);
					collision_buf++;
					break;
				}
				k = k->next;
			}
			l = l->next;
		}

		// move particles that has not collided with another
		l=head;
		while(l!= NULL) 
		{
			if (!l->collisions)
			{
				feuler(&(l->particle), 1);

				/* check for wall interaction and add the momentum */
				pressure_buf += wall_collide(&(l->particle), wall);
			}
			l = l->next;
		}
	}
	MPI_Reduce(&collision_buf, &collision, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&pressure_buf, &pressure, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

	clock_gettime(CLOCK_REALTIME, &etime);
	
	if (myrank == 0){
		printf("\nTotal collision(between particles) = %d\n",collision);
		printf("Total took: %g secs\n", (etime.tv_sec - stime.tv_sec) + 1e-9 * (etime.tv_nsec - stime.tv_nsec));
		printf("Average pressure = %f\n\n", pressure / (WALL_LENGTH * time_max));
	}

	/* MPI finialize */
	MPI_Finalize();

	return 0;
}
