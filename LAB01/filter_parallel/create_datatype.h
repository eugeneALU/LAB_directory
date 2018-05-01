# ifndef _CREATE_DATATYPE
# define _CREATE_DATATYPE

#include <mpi.h>

typedef struct _pixel
{
    unsigned char r, g, b;
} pixel;

void create_datatype_pixel(pixel* src ,MPI_Datatype* MPI_PIXEL);

# endif
