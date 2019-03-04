// GPU version of Matrix addition

#include <stdio.h>
#include <math.h>

const int N = 1024;
const int blocksize = (N>32)?32:N;           //MAX threads per block = 1024 Sqrt(1024)=32

__global__
void add_matrix(float *a, float *b, float *c, int N, int gridsize)
{
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int idy = blockIdx.y * blockDim.y + threadIdx.y;
  int id = idy*gridsize*blockDim.x + idx;
  if(id < N*N){
    c[id] = a[id] + b[id];
  }
}

int main()
{
  float *a = new float[N*N];
  float *b = new float[N*N];
  float *c = new float[N*N];
  float *a_g;
  float *b_g;
  float *c_g;
  float t;    //excution time in ms
  int gridsize;
  gridsize =(int)ceil((double)N/blocksize); //handle for the situation that N%blocksize != 0

  size_t size =  N*N*sizeof(float);
  cudaEvent_t start, end;
  /*
  cudaDeviceProp prop;
  cudaGetDeviceProperties(&prop, 0);
  printf("Name: %s\n",prop.name);
  printf("MAX Threads per block: %d\n", prop.maxThreadsPerBlock);
  printf("MAX Grid: [%d %d %d]\n", prop.maxGridSize[0],prop.maxGridSize[1],prop.maxGridSize[2]);
  printf("MAX shared Mem per block: %lu\n", prop.sharedMemPerBlock);
  //more property can be find : https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__DEVICE.html#group__CUDART__DEVICE_1g1bf9d625a931d657e08db2b4391170f0
  */
  cudaEventCreate(&start);
  cudaEventCreate(&end);
  cudaEventRecord(start, 0);
  cudaEventRecord(end, 0);

  dim3 dimBlock(blocksize, blocksize);
  dim3 dimGrid(gridsize, gridsize);
  cudaMalloc((void**)&a_g, size);
  cudaMalloc((void**)&b_g, size);
  cudaMalloc((void**)&c_g, size);

  for (int i = 0; i < N; i++){
    for (int j = 0; j < N; j++){
      a[i+j*N] = 10 + i;
      b[i+j*N] = (float)j / N;
    }
  }
  cudaMemcpy(a_g, a, size, cudaMemcpyHostToDevice);
  cudaMemcpy(b_g, b, size, cudaMemcpyHostToDevice);

  cudaEventSynchronize(start);
  add_matrix<<<dimGrid, dimBlock>>>(a_g, b_g, c_g, N, gridsize);
  cudaDeviceSynchronize();
  cudaEventSynchronize(end);
  cudaEventElapsedTime(&t, start, end);

  cudaMemcpy(c, c_g, size, cudaMemcpyDeviceToHost);
    
    int i;
    FILE *f = fopen("gpu.txt", "wb");
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
  delete[] a;
  delete[] b;
  delete[] c;
  cudaFree(a_g);
  cudaFree(b_g);
  cudaFree(c_g);

  printf("Cost %0.8f miliseconds\n", t);
  return EXIT_SUCCESS;
}
