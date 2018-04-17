#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "thresfilter.h"
#include <mpi.h>
#include "create_datatype.h"

int main (int argc, char ** argv) {
    int xsize, ysize, colmax;
    pixel *src = (pixel*) malloc(sizeof(pixel) * MAX_PIXELS);
    struct timespec stime, etime;

     /* MPI init */
    int myrank, n_task;
    pixel dummy;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); //get own rank number
    MPI_Comm_size(MPI_COMM_WORLD, &n_task);   //get total nuber of processor

    /* MPI data type create -- MPI_PIXEL */
    MPI_Datatype MPI_PIXEL;
    create_datatype_pixel(&dummy, &MPI_PIXEL);

    /* only rank0 process read the file */
    if (myrank == 0) {
        /* Take care of the arguments */
        if (argc != 3) {
        fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
        exit(1);
        }

        /* read file */
        if(read_ppm (argv[1], &xsize, &ysize, &colmax, (char *) src) != 0)
            exit(1);

        if (colmax > 255) {
        fprintf(stderr, "Too large maximum color-component value\n");
        exit(1);
        }

        printf("Has read the image, calling filter\n");
    }

    /* parallel take place */
    clock_gettime(CLOCK_REALTIME, &stime);

    int mean = 0;
    int total_size = 0;
    int chunk = 0;
    int remain = 0;

    if (myrank == 0){
        total_size = xsize * ysize;

        chunk = total_size / n_task;
        remain = total_size % n_task;
        mean = get_global_mean(xsize, ysize, src);
    }

    MPI_Bcast(&chunk,1,MPI_INT,0,MPI_COMM_WORLD);

    pixel *local_src = (pixel*) malloc(sizeof(pixel) * chunk);

    MPI_Bcast(&mean,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Scatter(src[remain],chunk,MPI_PIXEL,local_src,chunk,MPI_PIXEL,0,MPI_COMM_WORLD);

    if (myrank == 0) {
        thresfilter(remain, src, mean);
        thresfilter(chunk, local_src, mean);
    }
    else {
        thresfilter(chunk, local_src, mean);
    }

    MPI_Gather(local_src,chunk,MPI_PIXEL,src[remain],chunk,MPI_PIXEL,0,MPI_COMM_WORLD);

    clock_gettime(CLOCK_REALTIME, &etime);

    if (myrank == 0){
      printf("Filtering took: %g secs\n", (etime.tv_sec  - stime.tv_sec) +
  	   1e-9*(etime.tv_nsec  - stime.tv_nsec)) ;

      /* write result */
      printf("Writing output file\n");

      if(write_ppm (argv[2], xsize, ysize, (char *)src) != 0)
        exit(1);
    }

    /* MPI finialize */
    MPI_Finalize();

    return(0);
}
