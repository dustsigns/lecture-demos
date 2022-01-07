//Illustration of YCbCr color mixing
// Andreas Unterweger, 2018-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "common.hpp"

struct YCbCr_data
{
  const std::string window_name;
  
  static constexpr const char * const portion_names[] = { "Y", "Cb", "Cr" };
  
  YCbCr_data(const std::string &window_name)
   : window_name(window_name) { }
};

static cv::Mat GenerateColorImage(const cv::Vec3b &pixel_value)
{
  constexpr auto image_dimension = 300;
  const cv::Size image_size(image_dimension, image_dimension);
  const cv::Mat image(image_size, CV_8UC3, pixel_value);
  return image;
}

static std::string GetTrackbarName(const char * const component)
{
  using namespace std::string_literals;
  const auto trackbar_name = component + " portion"s;
  return trackbar_name;
}

static void UpdateImage(const int, void * const user_data)
{
  auto &data = *(static_cast<const YCbCr_data*>(user_data));
  unsigned char YCbCr_portions[comutils::arraysize(data.portion_names)];
  for (size_t i = 0; i < comutils::arraysize(data.portion_names); i++)
  {
    const auto trackbar_name = GetTrackbarName(data.portion_names[i]);
    const auto value = static_cast<unsigned char>(cv::getTrackbarPos(trackbar_name, data.window_name));
    YCbCr_portions[i] = value;
  }
  const cv::Vec3b pixel_value(YCbCr_portions[0], YCbCr_portions[2], YCbCr_portions[1]); //YCrCb order
  const cv::Mat ycrcb_image = GenerateColorImage(pixel_value);
  cv::Mat rgb_image;
  cv::cvtColor(ycrcb_image, rgb_image, cv::COLOR_YCrCb2BGR);
  cv::imshow(data.window_name, rgb_image);
}

static void ShowImage()
{
  constexpr auto window_name = "YCbCr color mixer";
  cv::namedWindow(window_name, cv::WINDOW_GUI_NORMAL);
  cv::moveWindow(window_name, 0, 0);
  static YCbCr_data data(window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  for (const auto &portion_name : data.portion_names)
  {
    const auto trackbar_name = GetTrackbarName(portion_name);
    cv::createTrackbar(trackbar_name, data.window_name, nullptr, 255, UpdateImage, static_cast<void*>(&data));
  }
  const auto y_trackbar_name = GetTrackbarName(data.portion_names[0]); //TODO: Make constexpr (together with GetTrackbarName, requires C++20)
  cv::setTrackbarPos(y_trackbar_name, window_name, 255); //Implies cv::imshow with white (Y=255, Cb=0, Cr=0)
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates how YCbCr portions can be mixed into different colors." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowImage();
  cv::waitKey(0);
  return 0;
}
