'''
    create data type for coordinate
'''
#include "create_datatype.h"

void create_datatype_particle(pcord_t* src ,MPI_Datatype* MPI_PARTICLE)
{
    int block_length[] = {1, 1, 1, 1};
    MPI_Datatype block_types[] = {
        MPI_FLOAT,
        MPI_FLOAT,
        MPI_FLOAT,
	MPI_FLOAT};
    MPI_Aint start, displace[4];

    MPI_Get_address(src, &start);
    MPI_Get_address(&(src->x), &displace[0]);
    MPI_Get_address(&(src->y), &displace[1]);
    MPI_Get_address(&(src->vx), &displace[2]);
    MPI_Get_address(&(src->vy), &displace[3]);

    displace[0] -= start;
    displace[1] -= start;
    displace[2] -= start;
    displace[3] -= start;

    MPI_Type_struct(4, block_length, displace, block_types, MPI_PARTICLE);
    MPI_Type_commit(MPI_PARTICLE);
}
