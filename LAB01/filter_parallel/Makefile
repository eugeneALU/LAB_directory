# Malefile for LAB01

CC = mpicc -Nmpi
FLAGS = -lrt

threshold: thresmain.c ppmio.c thresfilter.c create_datatype.c
	${CC} -o thresh thresmain.c ppmio.c thresfilter.c create_datatype.c ${FLAGS}

blur: blurmain.c ppmio.c blurfilter.c gaussw.c create_datatype.c
	${CC} -o blur blurmain.c ppmio.c blurfilter.c gaussw.c create_datatype.c ${FLAGS} -lm -g

