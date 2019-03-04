CPU_double:
	g++ milli.c interactiveMandelbrot.cpp -DDOUBLE -o interactiveMandelbrot_cpu_double -lglut -lGL

CPU:
	g++ milli.c interactiveMandelbrot.cpp -o interactiveMandelbrot_cpu -lglut -lGL

GPU_double:
	nvcc interactiveMandelbrot.cu -DDOUBLE -o interactiveMandelbrot_gpu_double -lglut -lGL

GPU:
	nvcc interactiveMandelbrot.cu -o interactiveMandelbrot_gpu -lglut -lGL

matrix_gpu:
	nvcc matrix_gpu.cu -o matrix_gpu

matrix_cpu:
	g++ milli.c matrix_cpu.c -o matrix_cpu

matrix_run:
	./matrix_gpu
	./matrix_cpu
