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

__global__ void filter(unsigned char *image, unsigned char *out, const unsigned int imagesizex, const unsigned int imagesizey, const int kernelsizex, const int kernelsizey)
{
    int localx = threadIdx.x;
    int localy = threadIdx.y;

    int dy, dx;
    unsigned int sumx, sumy, sumz;

    __shared__ unsigned char local_mem[BLOCKSIZE*3][BLOCKSIZE];
    int startx = (BLOCKSIZE-2*kernelsizex)*blockIdx.x;  //block start index
    int starty = (BLOCKSIZE-2*kernelsizey)*blockIdx.y;
    int realx = localx - kernelsizex + startx;          //real image idx that local idx corresponding to
    int realy = localy - kernelsizey + starty;  
    int xx = min(max(realx, 0), imagesizex-1);          //clamping
    int yy = min(max(realy, 0), imagesizey-1);

    local_mem[3*localx+0][localy] = image[((yy)*imagesizex+(xx))*3+0];
    local_mem[3*localx+1][localy] = image[((yy)*imagesizex+(xx))*3+1];
    local_mem[3*localx+2][localy] = image[((yy)*imagesizex+(xx))*3+2];
    __syncthreads();

	int divby = (2*kernelsizex+1)*(2*kernelsizey+1); // Works for box filters only!
    int x,y;
	// Filter kernel (simple box filter)
	sumx=0;sumy=0;sumz=0;
	for(dy=-kernelsizey;dy<=kernelsizey;dy++)
	{
		for(dx=-kernelsizex;dx<=kernelsizex;dx++)
		{
            x = min(max(localx+dx, 0), BLOCKSIZE-1);
            y = min(max(localy+dy, 0), BLOCKSIZE-1);

            sumx += local_mem[x*3+0][y];
            sumy += local_mem[x*3+1][y];
            sumz += local_mem[x*3+2][y];
		}
	}
    if (realx >= 0 && realx <= imagesizex - 1){
        if (realy >= 0 && realy <= imagesizey - 1){
            out[((yy)*imagesizex+(xx))*3+0] = sumx/divby;
            out[((yy)*imagesizex+(xx))*3+1] = sumy/divby;
            out[((yy)*imagesizex+(xx))*3+2] = sumz/divby;
            /*out[((yy)*imagesizex+(xx))*3+0] =local_mem[3*localx+0][localy];
            out[((yy)*imagesizex+(xx))*3+1] =local_mem[3*localx+1][localy];
            out[((yy)*imagesizex+(xx))*3+2] =local_mem[3*localx+2][localy];*/
        }
    }
}

// Global variables for image data
unsigned char *image, *pixels, *dev_bitmap, *dev_input;
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
    cudaMalloc( (void**)&dev_bitmap, imagesizex*imagesizey*3);
    printf("GRIDSIZE:%f\n", ceil(float(imagesizex)/(32-2*kernelsizex)));
    dim3 grid(ceil(float(imagesizex)/(BLOCKSIZE-2*kernelsizex)),ceil(float(imagesizey)/(BLOCKSIZE-2*kernelsizey)));
    dim3 block(BLOCKSIZE,BLOCKSIZE);
    ResetMilli();
    filter<<<grid,block>>>(dev_input, dev_bitmap, imagesizex, imagesizey, kernelsizex, kernelsizey); // change to blocksize = 32*32
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

	computeImages(7, 7);

	// You can save the result to a file like this:
	writeppm("out.ppm", imagesizey, imagesizex, pixels);

	glutMainLoop();
	return 0;
}
