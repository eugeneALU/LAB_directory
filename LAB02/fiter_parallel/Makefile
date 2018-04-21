# Malefile to thresmain.c
# compile with pthread

CC = gcc
FLAGS = -lpthread

threshold: thresmain.c ppmio.c thresfilter.c
	${CC} thresmain.c ppmio.c thresfilter.c ${FLAGS}

blur: blurmain.c ppmio.c blurfilter.c gaussw.c
	${CC} blurmain.c ppmio.c blurfilter.c gaussw.c ${FLAGS}
