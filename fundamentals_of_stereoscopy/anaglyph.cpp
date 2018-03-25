//Illustration of anaglyph images
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

static Mat GetAnaglyphImage(const Mat &left_image, const Mat &right_image)
{
  assert(left_image.size() == right_image.size());
  assert(left_image.type() == CV_8UC1);
  assert(right_image.type() == CV_8UC1);
  
  const Mat bgr_channels[3] { right_image, right_image, left_image }; //Set blue channel to right image and red channel to left image; set green channel to blue channel to not leave it empty
  Mat anaglyph_image(left_image.size(), CV_8UC3);
  merge(bgr_channels, 3, anaglyph_image);
  return anaglyph_image;
}

static void ShowImages(const Mat &left_image, const Mat &right_image)
{
  assert(left_image.size() == right_image.size());
  constexpr auto image_window_name = "Left and right images";
  namedWindow(image_window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(image_window_name, 0, 0);
  constexpr auto anaglyph_window_name = "Anaglyph image";
  namedWindow(anaglyph_window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(anaglyph_window_name, 2 * left_image.cols + 3 + 3, 0); //Move anaglyph window right beside the image window (2 images plus 3 border pixels plus additional distance)
  const Mat combined_image = CombineImages({left_image, right_image}, Horizontal);
  imshow(image_window_name, combined_image);
  const Mat anaglyph_image = GetAnaglyphImage(left_image, right_image);
  imshow(anaglyph_window_name, anaglyph_image);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    cout << "Illustrates stereoscopy with anaglyph images." << endl;
    cout << "Usage: " << argv[0] << " <left image> <right image>" << endl;
    return 1;
  }
  const auto left_image_filename = argv[1];
  const Mat left_image = imread(left_image_filename, IMREAD_GRAYSCALE);
  if (left_image.empty())
  {
    cerr << "Could not read left image '" << left_image_filename << "'" << endl;
    return 2;
  }
  const auto right_image_filename = argv[2];
  const Mat right_image = imread(right_image_filename, IMREAD_GRAYSCALE);
  if (right_image.empty())
  {
    cerr << "Could not read right image '" << right_image_filename << "'" << endl;
    return 3;
  }
  ShowImages(left_image, right_image);
  waitKey(0);
  return 0;
}
