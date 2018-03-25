//Illustration of semi-global stereo matching
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include "opencv2/core.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

static constexpr auto number_of_disparities = 64;
static constexpr auto number_of_fractional_bits = 4; //Sub-pixel precision of the stereo-matching algorithm in bits

static Mat ConvertDisparityImage(const Mat &disparity_image)
{
  constexpr auto number_of_output_values = 256; //8-bit unsigned
  constexpr auto number_of_input_values = number_of_disparities * (1 << number_of_fractional_bits);
  constexpr auto conversion_factor = static_cast<double>(number_of_output_values) / number_of_input_values;
  Mat converted_image;
  disparity_image.convertTo(converted_image, CV_8UC1, conversion_factor);
  return converted_image;
}

static Mat GetDisparityImage(const Mat &left_image, const Mat &right_image)
{
  assert(left_image.channels() == right_image.channels());
  const auto number_of_channels = left_image.channels();
  constexpr auto SAD_window_size = 5;
  const auto sgbm = StereoSGBM::create(0, number_of_disparities, SAD_window_size, 8 * number_of_channels * SAD_window_size * SAD_window_size, 32 * number_of_channels * SAD_window_size * SAD_window_size, 0, 0, 5, 50, 2, StereoSGBM::MODE_HH);
  Mat disparity_image;
  sgbm->compute(left_image, right_image, disparity_image);
  const Mat converted_disparity_image = ConvertDisparityImage(disparity_image);
  return converted_disparity_image;
}

static void ShowWindows(const Mat &left_image, const Mat &right_image)
{
  constexpr auto image_window_name = "Left and right images";
  namedWindow(image_window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(image_window_name, 0, 0);
  constexpr auto disparity_window_name = "Estimated disparity image";
  namedWindow(disparity_window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  assert(left_image.size() == right_image.size());
  moveWindow(disparity_window_name, 2 * left_image.cols + 3 + 3, 0); //Move anaglyph window right beside the image window (2 images plus 3 border pixels plus additional distance)
  const Mat combined_image = CombineImages({left_image, right_image}, Horizontal);
  imshow(image_window_name, combined_image);
  const Mat disparity_image = GetDisparityImage(left_image, right_image);
  imshow(disparity_window_name, disparity_image);
  waitKey(0);  
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    cout << "Illustrates the estimation of a disparity image via semi-global stereo matching." << endl;
    cout << "Usage: " << argv[0] << " <left image> <right image>" << endl;
    return 1;
  }
  const auto left_image_path = argv[1];
  const Mat left_image = imread(left_image_path, IMREAD_GRAYSCALE);
  if (left_image.empty())
  {
    cerr << "Could not read left image '" << left_image_path << "'" << endl;
    return 2;
  }
  const auto right_image_path = argv[2];
  const Mat right_image = imread(right_image_path, IMREAD_GRAYSCALE);
  if (right_image.empty())
  {
    cerr << "Could not read right image '" << right_image_path << "'" << endl;
    return 3;
  }
  ShowWindows(left_image, right_image);
  return 0;
}
