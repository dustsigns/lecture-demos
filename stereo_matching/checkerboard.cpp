//Illustration of a checkerboard pattern for calibration
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

using namespace std;

using namespace cv;

static Mat GenerateCheckerboardPattern(const unsigned int checkerboard_width, const unsigned int checkerboard_height)
{
  constexpr auto field_size = 60U;
  static_assert(field_size != 0, "The size of a checkerboard field cannot be zero.");
  
  constexpr auto black = 0U;
  constexpr auto white = 255U;
  
  Mat checkerboard(checkerboard_height * field_size, checkerboard_width * field_size, CV_8UC1, Scalar(black)); //Color whole board black
  for (unsigned int y = 0; y < checkerboard_height; y++)
  {
    for (unsigned int x = 0; x < checkerboard_width; x++)
    {
      if ((x ^ y) & 1) //Color every other field white
      {
        const Rect field_pixels(x * field_size, y * field_size, field_size, field_size);
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
  
  const auto window_name = to_string(checkerboard_width) + "x" + to_string(checkerboard_height) + " checkerboard";
  namedWindow(window_name, WINDOW_GUI_NORMAL);
  moveWindow(window_name, 0, 0);
  const Mat checkerboard_pattern = GenerateCheckerboardPattern(checkerboard_width, checkerboard_height);
  imshow(window_name, checkerboard_pattern);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates an asymmetrical checkerboard pattern for camera calibration." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  ShowCheckerboard();
  waitKey(0);
  return 0;
}
