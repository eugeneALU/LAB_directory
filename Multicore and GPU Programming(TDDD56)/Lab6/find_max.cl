/*
 * Placeholder OpenCL kernel
 */

__kernel void find_max(__global unsigned int *data, const unsigned int length)
{
  int global_size = get_global_size(0);
  int local_size = get_local_size(0);
  int group_id = get_group_id(0);
  int id = get_global_id(0);
  int idl = get_local_id(0);
  int i;

  for(i = ceil(local_size/2.0); i >= 1 ; i = i/2){
    if(idl < i){
      if (data[id] < data[id+i]){
        data[id] = data[id+i];
      }
    }
    barrier(CLK_LOCAL_MEM_FENCE);      //synchronize within group if we have more groups
  }
  if (idl == 0){
    data[group_id] = data[id];      //put the group-wise maximum to first group(if group_num < 512) for next run
  }
}
