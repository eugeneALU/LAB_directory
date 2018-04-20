#include "create_datatype.h"

void create_datatype_pixel(pixel *src, MPI_Datatype *MPI_PIXEL)
{
    int block_length[] = {1, 1, 1};
    MPI_Datatype block_types[] = {
        MPI_UNSIGNED_CHAR,
        MPI_UNSIGNED_CHAR,
        MPI_UNSIGNED_CHAR};
    MPI_Aint start, displace[3];

    MPI_Get_address(src, &start);
    MPI_Get_address(&(src->r), &displace[0]);
    MPI_Get_address(&(src->g), &displace[1]);
    MPI_Get_address(&(src->b), &displace[2]);

    displace[0] -= start;
    displace[1] -= start;
    displace[2] -= start;

    MPI_Type_struct(3, block_length, displace, block_types, MPI_PIXEL);
    MPI_Type_commit(MPI_PIXEL);
}
