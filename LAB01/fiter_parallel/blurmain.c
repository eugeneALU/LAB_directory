#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "blurfilter.h"
#include "gaussw.h"
#include <mpi.h>

int main(int argc, char **argv)
{
    int radius;
    int xsize, ysize, colmax;
    pixel *src = (pixel *)malloc(sizeof(pixel) * MAX_PIXELS);
    struct timespec stime, etime;
#define MAX_RAD 1000

    double w[MAX_RAD];

    /* MPI init */
    int myrank, n_task;
    pixel dummy; // just use to create MPI Datatype
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); //get own rank number
    MPI_Comm_size(MPI_COMM_WORLD, &n_task); //get total nuber of processor

    /* MPI data type create -- MPI_PIXEL */
    MPI_Datatype MPI_PIXEL;
    create_datatype_pixel(&dummy, &MPI_PIXEL);

    /* only rank0 process read the file */
    if (myrank == 0)
    {
        /* Take care of the arguments */
        if (argc != 4)
        {
            fprintf(stderr, "Usage: %s radius infile outfile\n", argv[0]);
            exit(1);
        }

        /* read file */
        if (read_ppm(argv[2], &xsize, &ysize, &colmax, (char *)src) != 0)
            exit(1);

        if (colmax > 255)
        {
            fprintf(stderr, "Too large maximum color-component value\n");
            exit(1);
        }
        printf("Has read the image, generating coefficients\n");

        printf("Calling filter\n");

        clock_gettime(CLOCK_REALTIME, &stime);
    }

    radius = atoi(argv[1]); //read in radius
    if ((radius > MAX_RAD) || (radius < 1))
    {
        fprintf(stderr, "Radius (%d) must be greater than zero and less then %d\n", radius, MAX_RAD);
        exit(1);
    }

    /* filter */
    get_gauss_weights(radius, w);

    /* parallel take place */

    int chunk = 0;
    int remain = 0;

    if (myrank == 0)
    {
        chunk = ysize / n_task;
        remain = ysize % n_task;
    }

    MPI_Bcast(&xsize, 1, MPI_INT, 0, MPI_COMM_WORLD); //brocast -- xsize
    MPI_Bcast(&chunk, 1, MPI_INT, 0, MPI_COMM_WORLD); //brocast -- chunk size

    pixel *local_src = (pixel *)malloc(sizeof(pixel) * (chunk + radius * 2) * xsize); //storage sending data
    int offset_line = 0; 
    int send_line = 0;
    int line, start_line, end_line, send_size, i;
    MPI_Status status[2];
    MPI_Request request[2];

    if (myrank == 0)
    {
        /* sending overlapping src */
        /*
            first {remain} lines go to process 0 so as first {chunk} lines 
            following {chunk} lines go to process 1 and so on till n_task
            each sending accompany with former and successive {radius} lines
        */
        for (i = 1; i < n_task; i++)
        {
            line = remain + chunk * i;
            start_line = line - radius; // send with previous {radius} line;
            if (start_line < 0)
            {
                start_line = 0; // if start_line exceed first line start from line 0
            }
            end_line = start_line + (chunk - 1) + radius;
            if (end_line > ysize - 1)
            {
                end_line = ysize - 1;
            }
	        send_line = end_line - start_line + 1;
	        MPI_Send(&send_line, 1, MPI_INT, i, 3, MPI_COMM_WORLD);

            //send_size = send_line * xsize;
            //send_size = (chunk + radius * 2) * xsize;
            offset_line = line - start_line;
            MPI_Isend(&src[start_line * xsize], send_line * xsize, MPI_PIXEL, i, 1, MPI_COMM_WORLD, &request[0]);
            MPI_Isend(&offset_line, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &request[1]);
            MPI_Waitall(2, request, status);
        }
    }
    else
    {
	MPI_Status status_size;
	MPI_Recv(&send_line, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, &status_size);   
	MPI_Irecv(local_src, send_line * xsize, MPI_PIXEL, 0, 1, MPI_COMM_WORLD, &request[0]);
        MPI_Irecv(&offset_line, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, &request[1]);
        MPI_Waitall(2, request, status);
    }

    /* apply filter */
    if (myrank == 0)
    {
        blurfilter(xsize, remain + chunk, src, radius, w, 0, remain+chunk+radius);
        //local_src = &src[remain * xsize];
        //offset_line = 0;
    }
    else
    {
        blurfilter(xsize, chunk, local_src, radius, w, offset_line, send_line);
    }

    /* gathering result */
    if (myrank != 0) 
    {
        MPI_Send(&local_src[offset_line * xsize], chunk * xsize, MPI_PIXEL, 0, 3, MPI_COMM_WORLD);
	//printf("current thread(%d) finish send\n", myrank);
    }
    else {
    	//MPI_Status *status_0 = (MPI_Status*)malloc(sizeof(MPI_Status)*(n_task-1));
    	//MPI_Request *request_0 = (MPI_Request*)malloc(sizeof(MPI_Request)* (n_task-1));
        for(i = 1; i < n_task; i++) {
	    MPI_Status status;
 	    line = remain + chunk * i;
	    MPI_Recv(&src[line*xsize], chunk * xsize, MPI_PIXEL, i, 3, MPI_COMM_WORLD, &status);
	    //MPI_Irecv(&src[line*xsize], chunk * xsize, MPI_PIXEL, i, 3, MPI_COMM_WORLD, &request_0[i-1]);
	    //printf("finish receive from thread(%d)\n", myrank);	
	}
	//MPI_Waitall(n_task-1, request_0, status_0);
        
    }	

    if (myrank == 0)
    {
        clock_gettime(CLOCK_REALTIME, &etime);

        printf("Filtering took: %g secs\n", (etime.tv_sec - stime.tv_sec) +
                                                1e-9 * (etime.tv_nsec - stime.tv_nsec));

        /* write result */
        printf("Writing output file\n");

        if (write_ppm(argv[3], xsize, ysize, (char *)src) != 0)
            exit(1);
    }

    /* MPI finalize */
    MPI_Finalize();

    return (0);
}
