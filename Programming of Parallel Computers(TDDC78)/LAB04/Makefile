#Makefile for LAB04

parallel:main_parallel.c  physics.c create_datatype.c
	mpicc -std=c99 -Nmpi -g -o $@ $^ -lrt -lm

sequential:main_sequential.c  physics.c
	icc -std=c99 -g -o $@ $^ -lrt -lm

debug:
	salloc -n3 mpprun --totalview ./parallel 100
run:
	salloc -n10 mpprun ./parallel 10

allclean:
	-rm *.o sequential
module:
	itac/8.1.3.037
