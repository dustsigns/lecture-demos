//Color constants for drawing with OpenCV (header)
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <opencv2/core.hpp>

namespace imgutils
{
  //Full red (RGB 255, 0, 0)
  const cv::Vec3b Red(0, 0, 255);
  //Full green (RGB 0, 255, 0)
  const cv::Vec3b Green(0, 255, 0);
  //Full blue (RGB 0, 0, 255)
  const cv::Vec3b Blue(255, 0, 0);
  
  //Full white (RGB 255, 255, 255)
  const cv::Vec3b White(255, 255, 255);
  //Full black (RGB 0, 0, 0)
  const cv::Vec3b Black(0, 0, 0);
  
  //Full purple (RGB 128, 0, 128)
  const cv::Vec3b Purple(128, 0, 128);
}
