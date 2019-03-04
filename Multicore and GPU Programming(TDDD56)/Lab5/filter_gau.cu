// Lab 5, image filters with CUDA.

// Compile with a command-line similar to Lab 4:
// nvcc filter.cu -c -arch=sm_30 -o filter.o
// g++ filter.o milli.c readppm.c -lGL -lm -lcuda -lcudart -L/usr/local/cuda/lib -lglut -o filter
// or (multicore lab)
// nvcc filter.cu -c -arch=sm_20 -o filter.o
// g++ filter.o milli.c readppm.c -lGL -lm -lcuda -L/usr/local/cuda/lib64 -lcudart -lglut -o filter

// 2017-11-27: Early pre-release, dubbed "beta".
// 2017-12-03: First official version! Brand new lab 5 based on the old lab 6.
// Better variable names, better prepared for some lab tasks. More changes may come
// but I call this version 1.0b2.
// 2017-12-04: Two fixes: Added command-lines (above), fixed a bug in computeImages
// that allocated too much memory. b3
// 2017-12-04: More fixes: Tightened up the kernel with edge clamping.
// Less code, nicer result (no borders). Cleaned up some messed up X and Y. b4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __APPLE__
  #include <GLUT/glut.h>
  #include <OpenGL/gl.h>
#else
  #include <GL/glut.h>
#endif
#include "readppm.h"
#include "milli.h"

// Use these for setting shared memory size.
#define maxKernelSizeX 10
#define maxKernelSizeY 10
#define BLOCKSIZE 32

// filter in x direction (imagesizex)
__global__ void filter(unsigned char *image, unsigned char *out, unsigned int imagesizex, unsigned int imagesizey,
                       const int kernelsize, const int intervalx, const int intervaly)
{
    int globalx = blockIdx.x * blockDim.x + threadIdx.x;
    int globaly = blockIdx.y * blockDim.y + threadIdx.y;
    int localx = threadIdx.x;
    int localy = threadIdx.y;

    //SWAP X and Y direction of global ID
    if (intervalx == imagesizex){
        globaly = blockIdx.x * blockDim.x + threadIdx.x;
        globalx = blockIdx.y * blockDim.y + threadIdx.y;
        localy = threadIdx.x;
        localx = threadIdx.y;
         //also need to swap image size incase non square input image, so input parameter has been swap
    }
    __shared__ unsigned char local_mem[(BLOCKSIZE+2*maxKernelSizeX)*3*BLOCKSIZE];

    int length = BLOCKSIZE+2*maxKernelSizeX;
    int offset = kernelsize;
    int xx = min(globalx, imagesizex-1);  //clamping
    int yy = min(globaly, imagesizey-1);
    int idx, d;

    local_mem[(localy*length+localx+offset)*3+0] = image[((yy)*intervaly+(xx)*intervalx)*3+0];
    local_mem[(localy*length+localx+offset)*3+1] = image[((yy)*intervaly+(xx)*intervalx)*3+1];
    local_mem[(localy*length+localx+offset)*3+2] = image[((yy)*intervaly+(xx)*intervalx)*3+2];

    int x;
    if(localx < kernelsize){
        x = max(xx-kernelsize,0);
        idx = yy*intervaly+x*intervalx;
        //idx = max(globaly*imagesizex+globalx - kernelsize*interval, 0);

        local_mem[(localy*length+localx)*3+0] = image[(idx)*3+0];
        local_mem[(localy*length+localx)*3+1] = image[(idx)*3+1];
        local_mem[(localy*length+localx)*3+2] = image[(idx)*3+2];
    }
    if (localx > BLOCKSIZE-1 - kernelsize){
        x = min(xx+kernelsize, imagesizex-1);
        idx = yy*intervaly+x*intervalx;
        //idx = min(globaly*imagesizex+globalx + kernelsize*interval, imagesizey*imagesizex);

        local_mem[(localy*length+localx+2*offset)*3+0] = image[(idx)*3+0];
        local_mem[(localy*length+localx+2*offset)*3+1] = image[(idx)*3+1];
        local_mem[(localy*length+localx+2*offset)*3+2] = image[(idx)*3+2];
    }
    __syncthreads();

    float weight[5] ={1.0/16,4.0/16,6.0/16,4.0/16,1.0/16};
    int i = 0;
    float sumx, sumy, sumz;

	// Filter kernel (simple box filter)
	sumx=0;sumy=0;sumz=0;
	for(d=-kernelsize;d<=kernelsize;d++)
	{
        //x = min(max(localx+d,0),BLOCKSIZE+kernelsize);
        //idx = localy*length+x+offset;
        idx = localy*length+localx+offset + d;

		sumx += local_mem[idx*3+0]*weight[i];
		sumy += local_mem[idx*3+1]*weight[i];
		sumz += local_mem[idx*3+2]*weight[i];

        i++;
	}

    out[((yy)*intervaly+(xx)*intervalx)*3+0] =  sumx;
    out[((yy)*intervaly+(xx)*intervalx)*3+1] =  sumy;
    out[((yy)*intervaly+(xx)*intervalx)*3+2] =  sumz;
}

// Global variables for image data
unsigned char *image, *pixels, *dev_bitmap, *dev_input, *intermediate;
unsigned int imagesizey, imagesizex; // Image size

////////////////////////////////////////////////////////////////////////////////
// main computation function
////////////////////////////////////////////////////////////////////////////////
void computeImages(int kernelsizex, int kernelsizey)
{
  double t;
	if (kernelsizex > maxKernelSizeX || kernelsizey > maxKernelSizeY)
	{
		printf("Kernel size out of bounds!\n");
		return;
	}

    pixels = (unsigned char *) malloc(imagesizex*imagesizey*3);
    cudaMalloc( (void**)&dev_input, imagesizex*imagesizey*3);
    cudaMemcpy( dev_input, image, imagesizey*imagesizex*3, cudaMemcpyHostToDevice );
    cudaMalloc( (void**)&intermediate, imagesizex*imagesizey*3);
    cudaMalloc( (void**)&dev_bitmap, imagesizex*imagesizey*3);

	dim3 grid(ceil(float(imagesizex)/(BLOCKSIZE)),ceil(float(imagesizey)/(BLOCKSIZE)));
	dim3 block(BLOCKSIZE,BLOCKSIZE); // change to blocksize = 32*32
    ResetMilli();
    // row wise, interval between each target = 1
    filter<<<grid,block>>>(dev_input, intermediate, imagesizex, imagesizey, kernelsizex, 1, imagesizex);
    cudaThreadSynchronize();
    // col wise, interval between each target = blocksize (=32)
    filter<<<grid,block>>>(intermediate, dev_bitmap, imagesizey, imagesizex, kernelsizey, imagesizex, 1);
    cudaThreadSynchronize();
    t = GetSeconds();
    printf("COST %lf seconds\n", t);
    //	Check for errors!
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
      printf("Error: %s\n", cudaGetErrorString(err));
    cudaMemcpy( pixels, dev_bitmap, imagesizey*imagesizex*3, cudaMemcpyDeviceToHost );
    cudaFree( dev_bitmap );
    cudaFree( dev_input );
}

// Display images
void Draw()
{
// Dump the whole picture onto the screen.
	glClearColor( 0.0, 0.0, 0.0, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );

	if (imagesizey >= imagesizex)
	{ // Not wide - probably square. Original left, result right.
		glRasterPos2f(-1, -1);
		glDrawPixels( imagesizex, imagesizey, GL_RGB, GL_UNSIGNED_BYTE, image );
		glRasterPos2i(0, -1);
		glDrawPixels( imagesizex, imagesizey, GL_RGB, GL_UNSIGNED_BYTE,  pixels);
	}
	else
	{ // Wide image! Original on top, result below.
		glRasterPos2f(-1, -1);
		glDrawPixels( imagesizex, imagesizey, GL_RGB, GL_UNSIGNED_BYTE, pixels );
		glRasterPos2i(-1, 0);
		glDrawPixels( imagesizex, imagesizey, GL_RGB, GL_UNSIGNED_BYTE, image );
	}
	glFlush();
}

// Main program, inits
int main( int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode( GLUT_SINGLE | GLUT_RGBA );

	if (argc > 1)
		image = readppm(argv[1], (int *)&imagesizex, (int *)&imagesizey);
	else
		image = readppm((char *)"maskros512.ppm", (int *)&imagesizex, (int *)&imagesizey);

	if (imagesizey >= imagesizex)
		glutInitWindowSize( imagesizex*2, imagesizey );
	else
		glutInitWindowSize( imagesizex, imagesizey*2 );
	glutCreateWindow("Lab 5");
	glutDisplayFunc(Draw);

	ResetMilli();

	computeImages(2, 2); // 1 * 5 gausian kernel

	// You can save the result to a file like this:
	writeppm("out.ppm", imagesizey, imagesizex, pixels);

	glutMainLoop();
	return 0;
}
