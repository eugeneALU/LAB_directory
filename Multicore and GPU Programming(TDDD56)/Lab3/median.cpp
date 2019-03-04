/**************************
** TDDD56 Lab 3
***************************
** Author:
** August Ernstsson
**************************/

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>
#include <iterator>

#include <skepu2.hpp>

#include "support.h"


unsigned char median_kernel(int ox, int oy, size_t stride, const unsigned char *image, size_t elemPerPx)
{
	int pixelNum = (2*oy+1)*(2*oy+1);
    int leftNum = 0, old_leftNum = 0, left, right;
    unsigned char Array[10000], tmp, median;
    int j = 0;
    int start = 0, end = pixelNum - 1;
    
    for (int y = -oy; y <= oy; ++y)
		for (int x = -ox; x <= ox; x += elemPerPx)
			Array[j++] = image[y*(int)stride+x]; 

    do{
// VERSION 1.0
        median = Array[end];
        left = start - 1;
        for (right = start; right <= end - 1; right++){
            if (Array[right] <= median){
                leftNum++;
                left++;
                tmp = Array[right];
                Array[right] = Array[left];
                Array[left] = tmp;
            }        
        }
        left++;
        tmp = Array[right];
        Array[right] = Array[left];
        Array[left] = tmp;

        if (leftNum > pixelNum/2){
            end = left - 1;
            leftNum = old_leftNum;
        }
        else if (leftNum < pixelNum/2){
            start = left + 1;
            leftNum++;                  //also add the median
            old_leftNum = leftNum;
        }     
        else {return median;} 

    }while(1);

    return median;
}



int main(int argc, char* argv[])
{
	LodePNGColorType colorType = LCT_RGB;
	
	if (argc < 5)
	{
		std::cout << "Usage: " << argv[0] << "input output radius [backend]\n";
		exit(1);
	}
	
	std::string inputFileName = argv[1];
	std::string outputFileName = argv[2];
	const int radius = atoi(argv[3]);
	auto spec = skepu2::BackendSpec{skepu2::Backend::typeFromString(argv[4])};
    spec.setCPUThreads(8);
	
	// Create the full path for writing the image.
	std::stringstream ss;
	ss << (2 * radius + 1) << "x" << (2 * radius + 1);
	std::string outputFileNamePad = outputFileName + ss.str() + "-median.png";
		
	// Read the padded image into a matrix. Create the output matrix without padding.
	ImageInfo imageInfo;
	skepu2::Matrix<unsigned char> inputMatrix = ReadAndPadPngFileToMatrix(inputFileName, radius, colorType, imageInfo);
	skepu2::Matrix<unsigned char> outputMatrix(imageInfo.height, imageInfo.width * imageInfo.elementsPerPixel, 120);
	
	// Skeleton instance
	auto calculateMedian = skepu2::MapOverlap(median_kernel);
	calculateMedian.setBackend(spec);
	calculateMedian.setOverlap(radius, radius  * imageInfo.elementsPerPixel);
	
	auto timeTaken = skepu2::benchmark::measureExecTime([&]
	{
		calculateMedian(outputMatrix, inputMatrix, imageInfo.elementsPerPixel);
	});

	WritePngFileMatrix(outputMatrix, outputFileNamePad, colorType, imageInfo);
	
	std::cout << "Time: " << (timeTaken.count() / 10E6) << "\n";
	
	return 0;
}


