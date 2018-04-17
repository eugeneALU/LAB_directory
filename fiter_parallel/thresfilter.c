#include "thresfilter.h"
#define uint unsigned int

void thresfilter(const int nump, pixel* src, const int mean){

  uint i, psum;
  /*
  for(i = 0, sum = 0; i < nump; i++) {
    sum += (uint)src[i].r + (uint)src[i].g + (uint)src[i].b;
  }

  sum /= nump;
  */

  for(i = 0; i < nump; i++) {
    psum = (uint)src[i].r + (uint)src[i].g + (uint)src[i].b;
    if(mean > psum) {
      src[i].r = src[i].g = src[i].b = 0;
    }
    else {
      src[i].r = src[i].g = src[i].b = 255;
    }
  }
}

uint get_global_mean(const int xsize, const int ysize, pixel* src){
  uint sum, i, nump;

  nump = xsize * ysize;

  for(i = 0, sum = 0; i < nump; i++) {
    sum += (uint)src[i].r + (uint)src[i].g + (uint)src[i].b;
  }
  sum /= nump;

  return sum;
}
