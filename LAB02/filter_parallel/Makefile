# Malefile to LAB02
# compile with pthread

CC = gcc
FLAGS = -lpthread -lrt

threshold: thresmain.c ppmio.c thresfilter.c
	${CC} -o thresh thresmain.c ppmio.c thresfilter.c ${FLAGS}

blur: blurmain.c ppmio.c blurfilter.c gaussw.c
	${CC} -o blur blurmain.c ppmio.c blurfilter.c gaussw.c ${FLAGS} -lm -g

