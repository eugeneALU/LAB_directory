/*
 * Placeholder OpenCL kernel
 */

__kernel void bitonic(__global unsigned int *data,int j, int k)
{
  int group_id = get_group_id(0);
  int id = get_global_id(0);
  int idl = get_local_id(0);
  unsigned int tmp;
  int ixj;

  ixj = id^j; // Calculate sibling index to compare, flip certain bit (+/- 1,2,4.....)

  if ((ixj)>id)
  {
    if ((id&k)==0 && data[id] > data[ixj]) {    // to form ascent part
      tmp = data[ixj];
      data[ixj] = data[id];
      data[id] = tmp;
    }
    if ((id&k)!=0 && data[id] < data[ixj]) {    // to form descent part
      tmp = data[ixj];
      data[ixj] = data[id];
      data[id] = tmp;
    }
  }
  barrier(CLK_GLOBAL_MEM_FENCE);      //synchronize within group
}

__kernel void bitonic_512(__global unsigned int *data,int length)
{
  int group_id = get_group_id(0);
  int id = get_global_id(0);
  int k,j;
  unsigned int tmp;
  int ixj;

   
  for (k=2;k<=length;k=2*k)
  {
    for (j=k>>1;j>0;j=j>>1)
    {
      ixj = id^j; // Calculate sibling index to compare, flip certain bit (+/- 1,2,4.....)
      if ((ixj)>id)
      {
        if ((id&k)==0 && data[id] > data[ixj]) {    // to form ascent part
          tmp = data[ixj];
          data[ixj] = data[id];
          data[id] = tmp;
        }
        if ((id&k)!=0 && data[id] < data[ixj]) {    // to form descent part
          tmp = data[ixj];
          data[ixj] = data[id];
          data[id] = tmp;
        }
      }
      barrier(CLK_GLOBAL_MEM_FENCE);      //synchronize within group
    }
  }

}
