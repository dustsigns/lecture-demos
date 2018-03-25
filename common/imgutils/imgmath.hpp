//Helper functions for calculations on images (header)
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <opencv2/core.hpp>

namespace imgutils
{
  using namespace cv;

  //Calculates the sum of absolute differences of a difference image
  double SAD(const Mat &difference_values);

  //Calculates the sum of squared differences of a difference image
  double SSD(const Mat &difference_values);

  //Calculates the mean squared error of a difference image
  double MSE(const Mat &difference_values);

  //Calculates the peak signal (maximum error) to noise ratio from the mean squared error
  double PSNR(const double MSE, const double max_error = 255);
  
  //Shifts all pixels of an unsigned 8-bit input image by half the range (128) and returns a 64-bit (double) output image
  Mat ImageLevelShift(const Mat &image);
  
  //Shifts a value by half of the 8-bit range (128)
  constexpr double LevelShift(const double value);
  //Shifts a value back by half of the 8-bit range (128)
  constexpr double ReverseLevelShift(const double value);
  
  //Shifts all pixels of a 64-bit (double) input image back by half the range (128) and returns an unsigned 8-bit output image
  Mat ReverseImageLevelShift(const Mat &image);
  
  //Generates a 64-bit (dobule) image of the 2-D-DCT basis function with indices (i, j) and the specified amplitude. The default amplitude is the maximum 8-bit amplitude of 255 from the range [0;255]
  Mat GetRaw2DDCTBasisFunctionImage(const unsigned int block_size, const unsigned int i, const unsigned int j, const double amplitude);
  //Generates an (unsigned) 8-bit image of the 2-D-DCT basis function with indices (i, j) and the specified amplitude. The default amplitude is the maximum 8-bit amplitude of 255 from the range [0;255] 
  Mat Get2DDCTBasisFunctionImage(const unsigned int block_size, const unsigned int i, const unsigned int j, const double amplitude = 255.0);
}

#include "imgmath.impl.hpp"
