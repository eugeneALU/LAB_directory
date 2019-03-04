ssh:
	ssh southfork-06.edu.isy.liu.se

filter_v1:
	nvcc filter_v1.cu -c -arch=sm_30 -o filter.o
	g++ filter.o milli.c readppm.c -lGL -lm -lcuda -lcudart -L/usr/local/cuda/lib -lglut -o filter_v1
	./filter_v1

filter_old:
	nvcc filter_old.cu -c -arch=sm_30 -o filter.o
	g++ filter.o milli.c readppm.c -lGL -lm -lcuda -lcudart -L/usr/local/cuda/lib -lglut -o filter_old
	./filter_old

filter_1D:
	nvcc filter_1D.cu -c -arch=sm_30 -o filter.o
	g++ filter.o milli.c readppm.c -lGL -lm -lcuda -lcudart -L/usr/local/cuda/lib -lglut -o filter_1D
	./filter_1D

filter_gau:
	nvcc filter_gau.cu -c -arch=sm_30 -o filter.o
	g++ filter.o milli.c readppm.c -lGL -lm -lcuda -lcudart -L/usr/local/cuda/lib -lglut -o filter_gau
	./filter_gau

filter_median:
	nvcc filter_median_v1.cu -c -arch=sm_30 -o filter.o
	g++ filter.o milli.c readppm.c -lGL -lm -lcuda -lcudart -L/usr/local/cuda/lib -lglut -o filter_median
	./filter_median maskros-noisy.ppm

filter_v2:
	nvcc filter_v2.cu -c -arch=sm_30 -o filter.o
	g++ filter.o milli.c readppm.c -lGL -lm -lcuda -lcudart -L/usr/local/cuda/lib -lglut -o filter_v2
	./filter_v2

