# Malefile to thresmain.c
# compile with pthread

CC = gcc
FLAGS = -lpthread

main: thresmain.c ppmio.c thresfilter.c
	${CC} thresmain.c ppmio.c thresfilter.c ${FLAGS}
