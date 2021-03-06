/*
  File: blurfilter.h

  Declaration of pixel structure and blurfilter function.
    
 */

#ifndef _BLURFILTER_H_
#define _BLURFILTER_H_
#include "create_datatype.h"

void blurfilter(const int xsize, const int ysize, pixel *src, const int radius, const double *w, const int offset_line, const int total_line);

#endif
