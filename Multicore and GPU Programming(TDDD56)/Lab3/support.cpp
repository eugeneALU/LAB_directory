#include <cmath>

#include "support.h"

// Reads a file from png and retuns it as a skepu::Matrix. Uses a library called LodePNG.
// Also returns information about the image as an out variable in imageInformation.
skepu2::Matrix<unsigned char> ReadPngFileToMatrix(std::string filePath, LodePNGColorType colorType, ImageInfo& imageInformation)
{
	std::vector<unsigned char> fileContents, image;
	unsigned imageWidth, imageHeight;
	unsigned error = lodepng::decode(image, imageWidth, imageHeight, filePath, colorType);
	if (error)
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	
	int elementsPerPixel = (colorType == LCT_GREY) ? 1 : 3;
	
	// Create a matrix which fits the image and the padding needed.
	skepu2::Matrix<unsigned char> inputMatrix(imageHeight, (imageWidth) * elementsPerPixel);
	
	int nonEdgeStartX = 0;
	int nonEdgeEndX = inputMatrix.total_cols();
	int nonEdgeStartY = 0;
	int nonEdgeEndY = inputMatrix.total_rows();
	
	// Initialize the inner real image values. The image is placed in the middle of the matrix, 
	// surrounded by the padding.
	for (int i = nonEdgeStartY; i < nonEdgeEndY; i++)
		for (int j = nonEdgeStartX; j < nonEdgeEndX; j++)
			inputMatrix(i, j)= image[(i - nonEdgeStartY) * imageWidth * elementsPerPixel + (j - nonEdgeStartX)];
	
	imageInformation.height = imageHeight;
	imageInformation.width = imageWidth;
	imageInformation.elementsPerPixel = elementsPerPixel;
	return inputMatrix;
}

// Reads a file from png and retuns it as a skepu::Matrix. Uses a library called LodePNG.
// Also returns information about the image as an out variable in imageInformation.
skepu2::Matrix<unsigned char> ReadAndPadPngFileToMatrix(std::string filePath, int kernelRadius, LodePNGColorType colorType, ImageInfo& imageInformation)
{
	std::vector<unsigned char> fileContents, image;
	unsigned imageWidth, imageHeight;
	unsigned error = lodepng::decode(image, imageWidth, imageHeight, filePath, colorType);
	if (error)
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	
	int elementsPerPixel = (colorType == LCT_GREY) ? 1 : 3;
	
	// Create a matrix which fits the image and the padding needed.
	skepu2::Matrix<unsigned char> inputMatrix(imageHeight + 2*kernelRadius, (imageWidth + 2*kernelRadius) * elementsPerPixel);
	
	int nonEdgeStartX = kernelRadius * elementsPerPixel;
	int nonEdgeEndX = inputMatrix.total_cols() - kernelRadius * elementsPerPixel;
	int nonEdgeStartY = kernelRadius;
	int nonEdgeEndY = inputMatrix.total_rows() - kernelRadius;
	
	// Initialize the inner real image values. The image is placed in the middle of the matrix, 
	// surrounded by the padding.
	for (int i = nonEdgeStartY; i < nonEdgeEndY; i++)
		for (int j = nonEdgeStartX; j < nonEdgeEndX; j++)
			inputMatrix(i, j)= image[(i - nonEdgeStartY) * imageWidth * elementsPerPixel + (j - nonEdgeStartX)];
	
	// Initialize padding. // Init topmost rows
	for (int row = 0;row < kernelRadius; row++)
	{
		for (int col = 0; col < inputMatrix.total_cols(); col++)
		{
			int minClampEdgeX = nonEdgeStartX + col % elementsPerPixel; 
			int maxClampEdgeX = nonEdgeEndX - elementsPerPixel + col % elementsPerPixel; 
			int xIndex = std::min(maxClampEdgeX, std::max(col, minClampEdgeX));
			int yIndex = std::min(nonEdgeEndY - 1, std::max(row, nonEdgeStartY));
			inputMatrix(row, col) = inputMatrix(yIndex, xIndex);
		}
	}
	
	// Init middle rows
	for (int row = kernelRadius; row < nonEdgeEndY; row++)
	{
		for (int col = 0; col < nonEdgeStartX; col++)
		{
			int minClampEdgeX = nonEdgeStartX + col % elementsPerPixel; 
			int maxClampEdgeX = nonEdgeEndX - elementsPerPixel + col % elementsPerPixel; 
			inputMatrix(row, col) = inputMatrix(row, minClampEdgeX);
			inputMatrix(row, col + nonEdgeEndX) = inputMatrix(row, maxClampEdgeX);
		}
	}
	
	// Init bottom rows
	for (int row = nonEdgeEndY; row < inputMatrix.total_rows(); row++)
	{
		for (int col = 0; col < inputMatrix.total_cols(); col++)
		{
			int minClampEdgeX = nonEdgeStartX + col % elementsPerPixel; 
			int maxClampEdgeX = nonEdgeEndX - elementsPerPixel + col % elementsPerPixel; 
			int xIndex = std::min(maxClampEdgeX, std::max(col, minClampEdgeX));
			int yIndex = std::min(nonEdgeEndY - 1, std::max(row, nonEdgeStartY));
			inputMatrix(row, col) = inputMatrix(yIndex, xIndex);
		}
	}
	
	imageInformation.height = imageHeight;
	imageInformation.width = imageWidth;
	imageInformation.elementsPerPixel = elementsPerPixel;
	return inputMatrix;
}

void WritePngFileMatrix(skepu2::Matrix<unsigned char> imageData, std::string filePath, LodePNGColorType colorType, ImageInfo& imageInformation)
{
	std::vector<unsigned char> imageDataVector; 
	for (int i = 0; i < imageData.total_rows(); i++)
		for (int j = 0; j < imageData.total_cols() ;j++)
			imageDataVector.push_back(imageData(i, j));
	
	unsigned error = lodepng::encode(filePath, &imageDataVector[0], imageInformation.width, imageInformation.height, colorType);
	if(error)
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}


skepu2::Vector<float> sampleGaussian(int radius)
{
	skepu2::Vector<float> vals(radius * 2 + 1);
	float s = radius / 3.0;
	for (int i = -radius; i <= radius; ++i)
	{
		double x = (float)i;
		vals[i+radius] = exp(-x*x / (2*s*s)) / sqrt(2*s*s*M_PI);
	}
	return vals;
}

