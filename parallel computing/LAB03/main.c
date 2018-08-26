#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define MAXITER 1000
#define N 1000

int main()
{
	double T[N + 2][N + 2] = {0};
	double tmp1[N + 1] = {0}, tmp2[N + 1] = {0};
	double error[20] = {0}, tol = 0.001, tmp_error;
	struct timespec stime, etime;
	int i, j, k;
	int r;

	double tmpEnd[N + 1] = {0};
	int n_thread = 0, id;
	int chunk = 0;
	int start, end;
	omp_set_dynamic(0); //disable dynamic threading

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
		#pragma omp parallel private(tmp_error, tmp2, id, start, end, j, k, i, tmp1, tmpEnd) shared(error, T, n_thread, chunk)
		{

			n_thread = omp_get_num_threads();
			chunk = N / n_thread;

			id = omp_get_thread_num();

			tmp_error = 0.0;
			error[id + 1] = 0.0;

			start = id * chunk + 1;
			end = (id + 1) * chunk;
			if (id == n_thread - 1)
			{
				end = N; //deal with the remaining rows
			}

			/* copy start line of every thread */
			for (i = 1; i < N + 1; i++)
			{
				tmp1[i] = T[i][start - 1];
			}

			/* copy (end+1) line of every thread */
			for (i = 1; i < N + 1; i++)
			{
				tmpEnd[i] = T[i][end + 1];
			}

			#pragma omp barrier    //in case some nodes are slower when copying data

			/* start counting new T */
			for (k = start; k <= end; k++)
			{
				/* copy current line */
				for (i = 0; i < N + 1; i++)
				{
					tmp2[i] = T[i][k];
				}
				/* renew T */
				for (j = 1; j < N + 1; j++)
				{
					if (k < end)
					{
						T[j][k] = (tmp2[j - 1] + T[j + 1][k] + T[j][k + 1] + tmp1[j]) / 4.0;
					}
					else
					{
						T[j][k] = (tmp2[j - 1] + T[j + 1][k] + tmpEnd[j] + tmp1[j]) / 4.0;
					}
				}
				/* tmp1 = tmp2 */
				for (i = 0; i < N + 1; i++)
				{
					tmp1[i] = tmp2[i];
				}
				/* caculate error */
				for (i = 1; i < N + 1; i++)
				{
					tmp_error = fabs(tmp2[i] - T[i][k]); //must use abs for float number (<<fabs>>)
					if (tmp_error > error[id + 1])
					{
						error[id + 1] = tmp_error;
					}
				}
			}
		}
		/* caculate the global error */
		error[0] = 0;
		for (i = 1; i <= n_thread; i++)
		{
			if (error[i] > error[0])
			{
				error[0] = error[i];
			}
		}

		/* early terminate */
		if (error[0] < tol)
		{
			break;
		}
		//printf("error:%lf\n", error[0]);
	}

	clock_gettime(CLOCK_REALTIME, &etime);
	printf("Total %d iterations took: %g secs\n", r,
		   (etime.tv_sec - stime.tv_sec) + 1e-9 * (etime.tv_nsec - stime.tv_nsec));
	printf("Temperature at T[1][1]: %.17f\n", T[1][1]);

	return 0;
}
