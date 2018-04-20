/*
  File: thresfilter.h

  Declaration of pixel structure and thresfilter function.
    
 */
#ifndef _THRESFILTER_H_
#define _THRESFILTER_H_
/* NOTE: This structure must not be padded! */
typedef struct _pixel {
    unsigned char r,g,b;
} pixel;

void thresfilter(const int nump, pixel* src, const int mean);
unsigned int get_global_mean(const int xsize, const int ysize, pixel* src);

#endif