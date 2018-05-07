#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define MAXITER 1000
#define N 1000

int main()
{
    double T[N + 2][N + 2] = {0.0};
    double tmp1[N + 1], tmp2[N + 1];
    double error, tol = 0.001, tmp_error;
    struct timespec stime, etime;
    int i, j, k;
    int r;
    //omp_set_nested(1);
    //omp_set_dynamic(1);

    /* set boundry condition */
    for (i = 0; i < N + 2; i++)
    {
        T[i][0] = 1.0;
        T[i][N + 1] = 1.0;
        T[N + 1][i] = 2.0;
    }

    clock_gettime(CLOCK_REALTIME, &stime);

    for (r = 1; r <= MAXITER; r++)
    {
        #pragma omp parallel private(i) shared(tmp1)
        {
            /* copy first line */
            #pragma omp for //ordered
            for (i = 1; i < N + 1; i++)
            {   
                //#pragma omp ordered
                tmp1[i] = T[i][0];
                
            }
        }
        //#pragma omp barrier --> without this still work (WHY?) this should be here
        #pragma omp parallel private(tmp_error,tmp2) firstprivate(tmp1) shared(error,T)
        {
            //#pragma omp single
            error = 0.0;
            tmp_error = 0.0;
            /* start counting new T */
            #pragma omp for collapse(2)//firstprivate(tmp1,tmp2)//ordered //schedule(dynamic) //reduction(max:error)
            for (k = 1; k < N + 1; k++)  
            {
                //#pragma omp for --> already collapse above
                for (j = 1; j < N + 1; j++)
                {
                    /* copy current line */
                    tmp2[j] = T[j][k];

                    /* renew T */
                    T[j][k] = (T[j - 1][k] + T[j + 1][k] + T[j][k + 1] + tmp1[j]) / 4.0;

                    /* tmp1 = tmp2 */
                    tmp1[j] = tmp2[j];

                    //#pragma omp barrier --> can't block in a parallelize task
                    
                    /* caculate error */
                    tmp_error = fabs(tmp2[j] - T[j][k]); //must use abs for float number (<<fabs>>)
                    //#pragma omp critical --> lead to worse performance
                    //#pragma omp ordered --> lead to worse performance
                    if (tmp_error > error)
                    {
                        //#pragma omp critical //--> faster but without it still work (WHY?)
                        error = tmp_error;
                    } 
                }
            }
        }

        /* early terminate */
        if (error < tol)
        {
            break;
        }
        printf("error:%lf\n", error);
    }

    clock_gettime(CLOCK_REALTIME, &etime);
    printf("Total %d iterations took: %g secs\n", r,
           (etime.tv_sec - stime.tv_sec) + 1e-9 * (etime.tv_nsec - stime.tv_nsec));
    printf("Temperature at T[1][1]: %f\n", T[1][1]);

    /*   FILE *f;
        f = fopen("result.txt", "w");
        for (i = 0; i < N + 2; i++)
        {
            for (j = 0; j <= N + 2; j++)
            {
                fprintf(f, "%f\n", T[i][j]);
            }
        }

        fclose(f);
    */
    return 0;
}