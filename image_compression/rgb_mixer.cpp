//Illustration of RGB color mixing
// Andreas Unterweger, 2018-2021
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"

using namespace std;

using namespace cv;

using namespace comutils;

struct RGB_data
{
  const string window_name;
  
  static constexpr char portion_names[] = { 'R', 'G', 'B' };
  
  RGB_data(const string &window_name)
   : window_name(window_name) { }
};

static Mat GenerateColorImage(const Vec3b &pixel_value)
{
  constexpr auto image_dimension = 300;
  const Size image_size(image_dimension, image_dimension);
  const Mat image(image_size, CV_8UC3, pixel_value);
  return image;
}

static string GetTrackbarName(const char component)
{
  const auto trackbar_name = component + " portion"s;
  return trackbar_name;
}

static void UpdateImage(const int, void * const user_data)
{
  auto &data = *(static_cast<const RGB_data*>(user_data));
  unsigned char RGB_portions[arraysize(data.portion_names)];
  for (size_t i = 0; i < arraysize(data.portion_names); i++)
  {
    const auto trackbar_name = GetTrackbarName(data.portion_names[i]);
    const auto value = static_cast<unsigned char>(getTrackbarPos(trackbar_name, data.window_name));
    RGB_portions[arraysize(RGB_portions) - (i + 1)] = value; //BGR order
  }
  const Vec3b pixel_value(RGB_portions);
  const Mat image = GenerateColorImage(pixel_value);
  imshow(data.window_name, image);
}

static void ShowImage()
{
  constexpr auto window_name = "RGB color mixer";
  namedWindow(window_name, WINDOW_GUI_NORMAL);
  moveWindow(window_name, 0, 0);
  static RGB_data data(window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  for (const auto &portion_name : data.portion_names)
  {
    const auto trackbar_name = GetTrackbarName(portion_name);
    createTrackbar(trackbar_name, data.window_name, nullptr, 255, UpdateImage, static_cast<void*>(&data));
  }
  const auto r_trackbar_name = GetTrackbarName(data.portion_names[0]); //TODO: Make constexpr (together with GetTrackbarName, requires C++20)
  setTrackbarPos(r_trackbar_name, window_name, 255); //Implies imshow with red (R=255, G=0, B=0)
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates how RGB portions can be mixed into different colors." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  ShowImage();
  waitKey(0);
  return 0;
}
