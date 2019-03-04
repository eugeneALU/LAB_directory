// Matrix addition, CPU version
// gcc matrix_cpu.c -o matrix_cpu -std=c99

#include <stdio.h>
#include "milli.h"

void add_matrix(float *a, float *b, float *c, int N)
{
	int index;

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
		{
			index = i + j*N;
			c[index] = a[index] + b[index];
		}
}

int main()
{
	const int N = 1024;

    float *a = new float[N*N];
    float *b = new float[N*N];
    float *c = new float[N*N];
    double t;

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
		{
			a[i+j*N] = 10 + i;
			b[i+j*N] = (float)j / N;
		}

	ResetMilli();
	add_matrix(a, b, c, N);
	t = GetSeconds();

    int i;
    FILE *f = fopen("cpu.txt", "wb");
    for (i = 0; i < N*N; i++) {
      fprintf(f, "%f\n", c[i]);
    }
    fclose(f);
/*
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			printf("%0.2f ", c[i+j*N]);
		}
		printf("\n");
	}*/

	printf("Cost %0.8f seconds\n", t);
}
