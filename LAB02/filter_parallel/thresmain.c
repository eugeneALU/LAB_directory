#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "thresfilter.h"
#include <pthread.h>

typedef struct
{
    int chunk;
    int n_thread;
    int x_size;
    int *mean;
    int remain;
    pixel *src;
} data;

int count = 0;

/* introduce barrier */
pthread_barrier_t barrier;
/* introduce lock */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *apply_filter(void *send_data)
{
    data *mydata = (data *)send_data;

    int chunk = mydata->chunk;
    int n_thread = mydata->n_thread;
    int *mean = mydata->mean;
    int remain = mydata->remain;
    pixel *src = mydata->src;

    pthread_mutex_lock(&lock);
    count++;
    int tmp = count;
    pthread_mutex_unlock(&lock);

    mean[tmp] = get_global_mean(chunk, &src[remain + chunk * tmp]);
    pthread_mutex_lock(&lock);
    mean[n_thread] += mean[tmp];
    pthread_mutex_unlock(&lock);

    pthread_barrier_wait(&barrier);

    int total_mean;
    total_mean = mean[n_thread] / n_thread;

    thresfilter(chunk, &src[remain + chunk * tmp], total_mean);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int xsize, ysize, colmax;
    pixel *src = (pixel *)malloc(sizeof(pixel) * MAX_PIXELS);
    struct timespec stime, etime;

    /* pthread init */
    int n_thread;
    pthread_t *thread;

    /* Take care of the arguments */
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s infile outfile thread_number\n", argv[0]);
        exit(1);
    }

    /* read file */
    if (read_ppm(argv[1], &xsize, &ysize, &colmax, (char *)src) != 0)
        exit(1);

    if (colmax > 255)
    {
        fprintf(stderr, "Too large maximum color-component value\n");
        exit(1);
    }

    /* read in thread number */
    n_thread = atoi(argv[3]);
    if (n_thread > 16 || n_thread < 1)
    {
        printf("Thread number is too big (0 < n_thread <= 16)\n");
        exit(-1);
    }
    thread = (pthread_t *)malloc(sizeof(pthread_t) * n_thread);
    if (thread == NULL)
    {
        printf("Out of memory!!!\n");
        exit(-1);
    }

    printf("Has read the image, calling filter\n");

    clock_gettime(CLOCK_REALTIME, &stime);

    /* parallel take place */
    int *mean = malloc(sizeof(int)* (n_thread+1));
    int total_size = 0;
    int chunk = 0;
    int remain = 0;
    int i;
    data send_data;
    pthread_barrier_init(&barrier,NULL,n_thread);

    mean[n_thread] = 0;
    total_size = xsize * ysize;
    chunk = total_size / n_thread;
    remain = total_size % n_thread;

    send_data.chunk = chunk;
    send_data.mean = mean;
    send_data.src = src;
    send_data.remain = remain;
    send_data.n_thread = n_thread;

    /* create thread */
    for (i = 1; i < n_thread; i++)
    {
        /* how to send data in for loop -- since here offset will be modify when I pass to the thread and cause wrong answer */
        //send_data.offset = malloc(sizeof(int));
        //send_data.offset = remain + chunk * i;
        if (pthread_create(&thread[i], NULL, apply_filter, (void *)&send_data) != 0)
        {
            printf("Error happen while creating thread(%d)\n", i + 1);
        }
    }

    mean[0] = get_global_mean(chunk+remain, &src[0]);
    mean[n_thread] += mean[0];

    pthread_barrier_wait(&barrier);

    int total_mean;
    total_mean = mean[n_thread] / n_thread;
    printf("THREAD(0):mean = %d\n",total_mean);

    /* main thread apply filter */
    thresfilter(remain, src, total_mean);
    thresfilter(chunk, &src[remain], total_mean);

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

    if (write_ppm(argv[2], xsize, ysize, (char *)src) != 0)
        exit(1);

    /* free memory */
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&lock);
    free(src);
    free(thread);

    return (0);
}
