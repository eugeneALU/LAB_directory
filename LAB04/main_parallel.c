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

//Feel free to change this program to facilitate parallelization.

float rand1()
{
	return (float)(rand() / (float)RAND_MAX);
}

void init_collisions(bool *collisions, unsigned int max)
{
	for (unsigned int i = 0; i < max; ++i)
		collisions[i] = 0;
}

int main(int argc, char **argv)
{

	unsigned int time_stamp = 0, time_max;
	float pressure = 0;
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

	// 2. allocate particle bufer and initialize the particles
	pcord_t *particles = (pcord_t *)malloc(INIT_NO_PARTICLES * sizeof(pcord_t));
	bool *collisions = (bool *)malloc(INIT_NO_PARTICLES * sizeof(bool));

	srand(time(NULL) + 1234);

	float r, a;
	for (int i = 0; i < INIT_NO_PARTICLES; i++)
	{
		// initialize random position
		particles[i].x = wall.x0 + rand1() * BOX_HORIZ_SIZE;
		particles[i].y = wall.y0 + rand1() * BOX_VERT_SIZE;

		// initialize random velocity
		r = rand1() * MAX_INITIAL_VELOCITY;
		a = rand1() * 2 * PI;
		particles[i].vx = r * cos(a);
		particles[i].vy = r * sin(a);
	}

	unsigned int p, pp;

	/* MPI init */
	int n_task;
	int dim[2] = {0};	 //2D Matrix without specified the 
	int period[2] = {0}; //without periodic 
	int reorder = 1; 	 //reorder
	MPI_Comm CART_COMM;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &n_task);	//get total nuber of processor
	MPI_Dims_create(n_task, 2, dim);	    //create proper 2 dimension
	MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &CART_COMM);	//create 2D topology

	int mycord[2] = {0};
	int myrank oldrank;


	MPI_Cart_get(CART_COMM, 2, dim, period, mycord);	//get personal coordinate 
	MPI_Cart_rank(CART_COMM, mycord, &myrank);			//get personal rank in new CART_COMM according to coordinate 
	MPI_Comm_rank(CART_COMM, &oldrank);

	printf("OLDRANK:%d NEWRANK:%d\n", oldrank, myrank);
	printf("rank(%d) cord: [%d , %d]\n",myrank, mycord[0], mycord[1]);


	clock_gettime(CLOCK_REALTIME, &stime);

	/* Main loop */
	for (time_stamp = 0; time_stamp < time_max; time_stamp++)
	{ // for each time stamp

		init_collisions(collisions, INIT_NO_PARTICLES);

		for (p = 0; p < INIT_NO_PARTICLES; p++) // for all particles
		{
			if (collisions[p])
				continue; //PURPOSE???

			/* check for collisions */
			for (pp = p + 1; pp < INIT_NO_PARTICLES; pp++)
			{
				if (collisions[pp])
					continue;
				float t = collide(&particles[p], &particles[pp]);
				if (t != -1) // collision
				{
					collisions[p] = collisions[pp] = 1;
					interact(&particles[p], &particles[pp], t);
					break; // only check collision of two particles
				}
			}
		}

		// move particles that has not collided with another
		for (p = 0; p < INIT_NO_PARTICLES; p++)
			if (!collisions[p])
			{
				feuler(&particles[p], 1);

				/* check for wall interaction and add the momentum */
				pressure += wall_collide(&particles[p], wall);
			}
	}

	MPI_Finalize();

	clock_gettime(CLOCK_REALTIME, &etime);

	printf("Total took: %g secs\n", (etime.tv_sec - stime.tv_sec) +
										1e-9 * (etime.tv_nsec - stime.tv_nsec));

	printf("Average pressure = %f\n", pressure / (WALL_LENGTH * time_max));

	free(particles);
	free(collisions);

	return 0;
}
