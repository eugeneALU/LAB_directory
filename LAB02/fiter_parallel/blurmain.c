#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "blurfilter.h"
#include "gaussw.h"
#include <pthread.h>

typedef struct
{
    int xsize;
    int chunk;
    int radius;
    int *offset_line;
    int *total_line;
    double *weight;
    pixel *copy;
    pixel *src;

} data;

int count = 0;

/* introduce lock */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *apply_filter(void *send_data)
{
    data *mydata = (data *) send_data;

    int xsize = mydata->xsize;
    int chunk = mydata->chunk;
    int radius = mydata->radius;
    double *w = mydata->weigth;
    pixel *copy = mydata->copy;
    pixel *src = mydata->src;

    pthread_mutex_lock(&lock);
    count++;
    int tmp = count;
    pthread_mutex_unlock(&lock);

    blurfilter(xsize, chunk, copy, src, radius, w, offset_line[tmp], total_line[tmp]);

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int radius;
    int xsize, ysize, colmax;
    pixel *src = (pixel *)malloc(sizeof(pixel) * MAX_PIXELS);
    struct timespec stime, etime;
#define MAX_RAD 1000

    double w[MAX_RAD];

    /* pthread init */
    int n_thread;
    pthread_t *thread;

    /* Take care of the arguments */
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s radius infile outfile thread_number\n", argv[0]);
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

    /* read in thread number */
    n_thread = atoi(argv[4]);
    if (n_thread > 16 || n_thread < 1)
    {
        printf("Thread number is too big (0 < n_thread <= 16)\n");
        exit(-1);
    }
    /* create container for thread */
    thread = (pthread_t *)malloc(sizeof(pthread_t) * n_thread);
    if (thread == NULL)
    {
        printf("Out of memory!!!\n");
        exit(-1);
    }

    radius = atoi(argv[1]); //read in radius
    if ((radius > MAX_RAD) || (radius < 1))
    {
        fprintf(stderr, "Radius (%d) must be greater than zero and less then %d\n", radius, MAX_RAD);
        exit(1);
    }

    /* get filter weight */
    get_gauss_weights(radius, w);

    /* parallel take place */
    data send_data;
    int chunk = 0;
    int remain = 0;
    int line, start_line, end_line, send_size, i;
    int *offset_line = (int *)malloc(sizeof(int)*n_task);
    int *total_line = (int *)malloc(sizeof(int)*n_task);
    pixel *copy = (pixel *)malloc(sizeof(pixel) * ysize * xsize); //copy one src
    for (i = 0; i < ysize*xsizel; i++){
        copy[i] = src[i];
    }

    chunk = ysize / n_task;
    remain = ysize % n_task;

    send_data.xsize = xsize;
    send_data.chunk = chunk;
    send_data.radius = radius;
    send_data.copy = copy;
    send_data.total_line = total_line;
    send_data.offset_line = offset_line;
    send_data.src = src;
    send_data.copy = copy;

    /* create thread */
    for (i = 1; i < n_thread; i++)
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
        
        total_line[i] = end_line - start_line + 1;
        offset_line[i] = line;
        if (pthread_create(&thread[i], NULL, apply_filter, (void *)&send_data)!=0)
        {
            printf("Error happen while creating thread(%d)\n", i + 1);
        }
    }

    /* apply filter (main thread)*/
    blurfilter(xsize, remain + chunk, copy, src, radius, w, 0, remain+chunk+radius);

    /* wait until thread finish */
    for (i = 1; i < n_thread; i++)
    {
        pthread_join(thread[i], NULL);
    }

    clock_gettime(CLOCK_REALTIME, &etime);

    printf("Filtering took: %g secs\n", (etime.tv_sec - stime.tv_sec) +
                                            1e-9 * (etime.tv_nsec - stime.tv_nsec));

    /* write result */
    printf("Writing output file\n");

    if (write_ppm(argv[3], xsize, ysize, (char *)src) != 0)
        exit(1);

    /* MPI finalize */
    MPI_Finalize();

    return (0);
}
