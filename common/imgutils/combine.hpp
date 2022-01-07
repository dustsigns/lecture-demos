//Image combination functions (header)
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <cstddef>

#include <opencv2/core.hpp>

namespace imgutils
{
  //Position of combined images
  enum CombinationMode { Horizontal, Vertical };

  //Method to convert difference images
  enum DifferenceConversionMode
  {
    Offset, //Adds 128 to the difference, clipping values below -128 and above 127, respectively
    Absolute, //Calculates absolute differences, i.e., without a sign
    Color //Converts positive differences to red values and negative differences to blue values
  };

  //Concatenates N images (of potentially different sizes and color spaces) horizontally or vertically with black borders between them. If the images differ in size, they are border-filled to the largest width and height across all images.
  template<size_t N>
  cv::Mat CombineImages(const cv::Mat (&images)[N], const CombinationMode mode, const unsigned int border_size = 3);

  //Concatenates N images (of potentially different sizes and color spaces) horizontally or vertically with black borders between them. If the images differ in size, they are border-filled to the largest width and height across all images.
  cv::Mat CombineImages(const size_t N, const cv::Mat images[], const CombinationMode mode, const unsigned int border_size = 3);

  //Subtracts two (unsigned) 8-bit images from one another and returns a (signed) 16-bit difference image
  cv::Mat SubtractImages(const cv::Mat &image1, const cv::Mat &image2);

  //Converts a difference image (with one channel of signed 16-bit values) to illustrate it as an unsigned 8-bit image, e.g., with imshow
  cv::Mat ConvertDifferenceImage(const cv::Mat &difference_image, DifferenceConversionMode mode = Color);
}

#include "combine.impl.hpp"
