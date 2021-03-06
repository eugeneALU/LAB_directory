#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define MAXITER 1000
#define N 1000

int main()
{
    double T[N + 2][N + 2] = {0.0};
    double tmpT[N + 2][N + 2] = {0.0};
    double tmp1[N + 1], tmp2[N + 1];
    double error, tol = 0.001, tmp_error;
    struct timespec stime, etime;
    int i, j, k;
    int r;

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
        /* copy first line */
        for (i = 1; i < N + 1; i++)
        {
            tmp1[i] = T[i][0];
        }
        error = 0.0;
        tmp_error = 0.0;

        /* start counting new T */
        for (k = 1; k < N + 1; k++)
        {
            /* copy current line */
            for (i = 1; i < N + 1; i++)
            {
                tmp2[i] = T[i][k];

                /* renew T */
                tmpT[i][k] = (T[i - 1][k] + T[i + 1][k] + T[i][k + 1] + tmp1[i]) / 4.0;

                /* tmp1 = tmp2 */
                tmp1[i] = tmp2[i];

                /* caculate error */
                tmp_error = fabs(tmp2[i] - tmpT[i][k]); //must use abs for float number (<<fabs>>)
                if (tmp_error > error)
                {
                    error = tmp_error;
                }
            }
        }
        for (k = 1; k < N + 1; k++)
        {
            for (i = 1; i < N + 1; i++)
            {
                T[i][k] = tmpT[i][k];
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
    return 0;
}
