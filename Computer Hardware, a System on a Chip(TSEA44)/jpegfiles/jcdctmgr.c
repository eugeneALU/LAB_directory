/*
 * jcdctmgr.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the forward-DCT management logic.
 * This code selects a particular DCT implementation to be used,
 * and it performs related housekeeping chores including coefficient
 * quantization.
 * 
 * Modified for the TSEA44 course
 */

#include <stdio.h>
#include <stdlib.h>

#include "my_encoder.h"
#include "jdct.h"
#include "perfctr.h"

/* Private subobject for this module */
static int row; // The row of the first pixel in the current MCU
static int col; // The column of the first pixel the current MCU
static unsigned int width; // Image width
static unsigned int height; // Image Height
static unsigned char  *theimage; // The raw image

/* Quantization matrix, Matlab notation
Q = [16 11 10 16 24 40 51 61;
     12 12 14 19 26 58 60 55; 
     14 13 16 24 40 57 69 56;
     14 17 22 29 51 87 80 62;
     18 22 37 56 68 109 103 77;
     24 35 55 64 81 104 113 92;
     49 64 78 87 103 121 120 101; 
     72 92 95 98 112 100 103 99];

reciprocals = round(2^15 ./ Q);
*/

static const int reciprocals[] = {2048, 2979, 3277, 2048, 1365, 819, 643, 537,
				  2731, 2731, 2341, 1725, 1260, 565, 546, 596,
				  2341, 2521, 2048, 1365,  819, 575, 475, 585,
				  2341, 1928, 1489, 1130,  643, 377, 410, 529,
				  1820, 1489,  886,  585,  482, 301, 318, 426,
				  1365,  936,  596,  512,  405, 315, 290, 356,
				  669,   512,  420,  377,  318, 271, 273, 324,
				  455,   356,  345,  334,  293, 328, 318, 331};


int workspace[DCTSIZE2];

void init_image(unsigned char *t,unsigned int image_width, unsigned int image_height)
{
   theimage = t;
   row = 0;
   col = 0;
   width = image_width;
   height = image_height;
}


/*
 * forward DCT
 *
 * 1) Copy a block from theimage to workspace and subtract 128
 * 2) DCT
 * 3) Quantization
 *
 */
void forward_DCT (short coef_block[DCTSIZE2])
{
  int *pw = workspace;
  unsigned char *pb = theimage + row*width + col;
  int *pim = (int *) pb;
  int *pr=reciprocals;
  short *pc=coef_block;
  int y,x; 											// The current position within the MCU
  int temp, i, j, rval;
  unsigned int startcycle = gettimer();

#ifdef HW_DMA
  #ifdef HW_DCT
	int DONE = 0, result;
	int trans[8][8];
	int ADDR_OUT = 0x96000800;

	// 1) Wait for DMA_DCT_Q to complete a block
	while((DONE & 0x00000002) != 0x00000002){
		DONE = REG32(0x96001810);
	}

	// 2) Read out data, transpose, convert from 16 to 32 bit
	for (i=0; i < DCTSIZE; i++) {
		for (j=0; j < DCTSIZE; j+=2) {
			result = REG32(ADDR_OUT);
			ADDR_OUT += 4;
			trans[i][j] = result >> 16;						//top 16
			trans[i][j+1] = (result << 16) >> 16;			//bot 16
		}
	}
	for (i=0; i < DCTSIZE; i++) {
		for (j=0; j < DCTSIZE; j++) {
			*pc++ = (short)trans[j][i];
		}
	}

	// 3) Continue with the next block
	REG32(0x96001810) = 2;			//startnextblock signal in jpeg_dma.sv

  #endif
#else
  #ifdef HW_DCT
	int DONE = 0,result;
	int ADDR_IN = 0x96000000;
	int ADDR_OUT = 0x96000800;
	int trans[8][8];
	int DCT_TIME;
	int *pd_int = (int*)pb; 

  	// 1) copy values from image to block RAM instead
  	// 2) subtract 128 in SW
	for (y = 0; y < DCTSIZE; y++, pd_int += (width - DCTSIZE)/4) {			// (.../4) since our step now is 4 times the unsigned char pointer
		for (x = 0; x < DCTSIZE; x+=4) {
			REG32(ADDR_IN) = *pd_int;	
			pd_int++;						       // jump 4 char since now it is int pointer
			ADDR_IN += 4;
		}
	}
	
	col += DCTSIZE;
	if (col >= width){
		col = 0;
		row += DCTSIZE;
	}
	perf_copy += gettimer() - startcycle;

 	// 3) start DCT_Q
	REG32(0x96001000) = 0x01000000;

 	// 4) wait for it to finish
	while(DONE != 0x80000000){
		DONE = REG32(0x96001000);
	}

 	// 5) read out, transpose, convert from 16 to 32 bit 				
	for (i=0; i < DCTSIZE; i++) {
		for (j=0; j < DCTSIZE; j+=2) {
			result = REG32(ADDR_OUT);
			ADDR_OUT += 4;
			trans[i][j] = result >> 16;				//top 16
			trans[i][j+1] = (result << 16) >> 16;			//bot 16
		}
	}
	for (i=0; i < DCTSIZE; i++) {
		for (j=0; j < DCTSIZE; j++) {
			*pc++ = (short)trans[j][i];
		}
	}
  #else
	// 1) Load data into workspace, applying unsigned->signed conversion
	// 2) subtract 128 (JPEG)
	for (y = 0; y < DCTSIZE; y++, pb += (width - DCTSIZE)) {
		for (x = 0; x < DCTSIZE; x++) {
		  *pw++ = (int) *pb++ - 128;			// convert *p which is unsigned char to int
		}
	}
	col += DCTSIZE;
	if (col >= width){
		col = 0;
		row += DCTSIZE;
	}
	perf_copy += gettimer() - startcycle;

	// 3) Perform the DCT       
	jpeg_fdct_islow (workspace);

	// 4) Quantize/descale the coefficients, and store into coef_blocks[]
	int rnd,pos,bits;
	for (i=0, pw=workspace; i < DCTSIZE2; i++) {
		rval = *pr++;
		temp = *pw++;
		  
		temp = temp*rval;
		  
		rnd = (temp & 0x10000) != 0 ; 
		bits = (temp & 0xffff) != 0; 
		pos = (temp & 0x80000000) == 0; 
		temp = temp >> 17; 
		temp += rnd && (pos || bits);
		*pc++ = (short) temp;
	}
  #endif
#endif
	perf_dct += gettimer() - startcycle;

}


/* This is the main encoding loop */ 
void encode_image(void)
{
	int i;
	int MCU_count = width * height / DCTSIZE2;
	short MCU_block[DCTSIZE2];

	printf("Width: %d  Hegith: %d\n", width, height);
	for(i = 0; i < MCU_count; i++)
	{
		forward_DCT(MCU_block);
		encode_mcu_huff(MCU_block);
	}
}

/* Initialize the encoder */
void init_encoder(int width,int height,unsigned char *image, FILE *fp)
{
  init_huffman(fp,width,height);
  init_image(image, width, height);
}
