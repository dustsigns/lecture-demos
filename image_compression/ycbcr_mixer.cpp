//Illustration of YCbCr color mixing
// Andreas Unterweger, 2018-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "common.hpp"

using namespace std;

using namespace cv;

using namespace comutils;

struct YCbCr_data
{
  int portions[3];
  
  const string window_name;
  
  YCbCr_data(const string &window_name)
   : portions{0, 0, 0},
     window_name(window_name) { }
};

static Mat GenerateColorImage(const Vec3b &pixel_value)
{
  constexpr auto image_dimension = 300;
  const Size image_size(image_dimension, image_dimension);
  const Mat image(image_size, CV_8UC3, pixel_value);
  return image;
}

static void UpdateImage(const int, void * const user_data)
{
  auto &data = *(static_cast<const YCbCr_data*>(user_data));
  for (const auto &portion : data.portions)
    assert(portion >= 0 && portion <= 255); //TODO: Validate actual range
  const Vec3b pixel_value(static_cast<unsigned char>(data.portions[0]), static_cast<unsigned char>(data.portions[2]), static_cast<unsigned char>(data.portions[1])); //YCrCb order
  const Mat ycrcb_image = GenerateColorImage(pixel_value);
  Mat rgb_image;
  cvtColor(ycrcb_image, rgb_image, COLOR_YCrCb2BGR);
  imshow(data.window_name, rgb_image);
}

static string GetTrackbarName(const char * const component)
{
  const auto trackbar_name = component + " portion"s;
  return trackbar_name;
}

static void ShowImage()
{
  constexpr auto window_name = "YCbCr color mixer";
  namedWindow(window_name, WINDOW_GUI_NORMAL);
  moveWindow(window_name, 0, 0);
  static YCbCr_data data(window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  constexpr const char * const YCbCr_names[] = { "Y", "Cb", "Cr" };
  for (size_t i = 0; i < arraysize(YCbCr_names); i++)
  {
    const auto trackbar_name = GetTrackbarName(YCbCr_names[i]);
    static_assert(arraysize(YCbCr_names) == arraysize(data.portions), "YCbCr_data::portions must have the same size as the YCbCr names array");
    createTrackbar(trackbar_name, window_name, &data.portions[i], 255, UpdateImage, static_cast<void*>(&data));
  }
  const auto y_trackbar_name = GetTrackbarName(YCbCr_names[0]); //TODO: Make constexpr (together with GetTrackbarName, requires C++20)
  setTrackbarPos(y_trackbar_name, window_name, 255); //Implies imshow with white (Y=255, Cb=0, Cr=0)
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates how YCbCr portions can be mixed into different colors." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  ShowImage();
  waitKey(0);
  return 0;
}
