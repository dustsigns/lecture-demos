//Illustration of a checkerboard pattern for calibration
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <string>

#include <opencv2/core.hpp>

#include "window.hpp"

static cv::Mat GenerateCheckerboardPattern(const unsigned int checkerboard_width, const unsigned int checkerboard_height)
{
  constexpr auto field_size = 60U;
  static_assert(field_size != 0, "The size of a checkerboard field cannot be zero.");
  
  constexpr auto black = 0U;
  constexpr auto white = 255U;
  
  cv::Mat checkerboard(checkerboard_height * field_size, checkerboard_width * field_size, CV_8UC1, cv::Scalar(black)); //Color whole board black
  for (unsigned int y = 0; y < checkerboard_height; y++)
  {
    for (unsigned int x = 0; x < checkerboard_width; x++)
    {
      if ((x ^ y) & 1) //Color every other field white
      {
        const cv::Rect field_pixels(x * field_size, y * field_size, field_size, field_size);
        checkerboard(field_pixels).setTo(white);
      }
    }
  }
  return checkerboard;
}
 
static void ShowCheckerboard()
{
  constexpr auto checkerboard_width = 10U;
  constexpr auto checkerboard_height = 7U;
  static_assert(checkerboard_width > 0 && checkerboard_height > 0, "The size of the checkerboard cannot be zero.");
  static_assert(checkerboard_width != checkerboard_height, "The checkerboard needs to be asymmetric.");
  
  const auto window_name = std::to_string(checkerboard_width) + "x" + std::to_string(checkerboard_height) + " checkerboard";
  const cv::Mat checkerboard_pattern = GenerateCheckerboardPattern(checkerboard_width, checkerboard_height);
  imgutils::Window window(window_name, checkerboard_pattern);
  window.ShowInteractive();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates an asymmetrical checkerboard pattern for camera calibration." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowCheckerboard();
  return 0;
}
