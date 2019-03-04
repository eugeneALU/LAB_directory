#ifndef __INGEMARS_CL_UTILITIES_
#define __INGEMARS_CL_UTILITIES_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
  #include <OpenCL/opencl.h>
#else
  #include <CL/cl.h>
#endif

// global variables needed after initialization
cl_context cxGPUContext;
cl_command_queue commandQueue;

// Convenient global (from clGetDeviceInfo)
int gMaxThreadsPerWG;

char* readFile(const char * filename);
void printCLError(cl_int ciErrNum, int location);

int initOpenCL();
cl_kernel compileKernel(char *filename, char *kernelName);
void closeOpenCL();

#endif

