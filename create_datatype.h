# ifndef _CREATE_DATATYPE
# define _CREATE_DATATYPE

#include <mpi.h>
/*
typedef struct {
    unsigned char r,g,b;
} pixel_struct;
*/
void create_datatype_pixel(struct pixel* src ,MPI_Datatype* MPI_PIXEL);

# endif