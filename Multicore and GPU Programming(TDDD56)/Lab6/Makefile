ssh:
	ssh southfork-00.edu.isy.liu.se

Hello:
	gcc hello_world_cl.c CLutilities.c -lOpenCL -I/usr/local/cuda/include -o hello_world_cl
	./hello_world_cl

max:
	gcc -std=c99 find_max.c -o find_max milli.c CLutilities.c -lOpenCL  -I/usr/local/cuda/include/
	./find_max

sort:
	gcc -std=c99 bitonic.c -o sort milli.c CLutilities.c -lOpenCL  -I/usr/local/cuda/include/ -lm
	./sort
