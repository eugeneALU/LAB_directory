/*
  File: blurfilter.h

  Declaration of pixel structure and blurfilter function.
    
 */

#ifndef _BLURFILTER_H_
#define _BLURFILTER_H_

/* NOTE: This structure must not be padded! */
typedef struct _pixel
{
    unsigned char r, g, b;
} pixel;

void blurfilter(const int xsize, const int ysize, pixel *copy, pixel *src, const int radius, 
                const double *w, const int offset_line, const int y_max);
                
#endif
