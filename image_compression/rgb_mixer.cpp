//Illustration of RGB color mixing
// Andreas Unterweger, 2018-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"

struct RGB_data
{
  const std::string window_name;
  
  static constexpr char portion_names[] = { 'R', 'G', 'B' };
  
  RGB_data(const std::string &window_name)
   : window_name(window_name) { }
};

static cv::Mat GenerateColorImage(const cv::Vec3b &pixel_value)
{
  constexpr auto image_dimension = 300;
  const cv::Size image_size(image_dimension, image_dimension);
  const cv::Mat image(image_size, CV_8UC3, pixel_value);
  return image;
}

static std::string GetTrackbarName(const char component)
{
  using namespace std::string_literals;
  const auto trackbar_name = component + " portion"s;
  return trackbar_name;
}

static void UpdateImage(const int, void * const user_data)
{
  auto &data = *(static_cast<const RGB_data*>(user_data));
  unsigned char RGB_portions[comutils::arraysize(data.portion_names)];
  for (size_t i = 0; i < comutils::arraysize(data.portion_names); i++)
  {
    const auto trackbar_name = GetTrackbarName(data.portion_names[i]);
    const auto value = static_cast<unsigned char>(cv::getTrackbarPos(trackbar_name, data.window_name));
    RGB_portions[comutils::arraysize(RGB_portions) - (i + 1)] = value; //BGR order
  }
  const cv::Vec3b pixel_value(RGB_portions);
  const cv::Mat image = GenerateColorImage(pixel_value);
  cv::imshow(data.window_name, image);
}

static void ShowImage()
{
  constexpr auto window_name = "RGB color mixer";
  cv::namedWindow(window_name, cv::WINDOW_GUI_NORMAL);
  cv::moveWindow(window_name, 0, 0);
  static RGB_data data(window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  for (const auto &portion_name : data.portion_names)
  {
    const auto trackbar_name = GetTrackbarName(portion_name);
    cv::createTrackbar(trackbar_name, data.window_name, nullptr, 255, UpdateImage, static_cast<void*>(&data));
  }
  const auto r_trackbar_name = GetTrackbarName(data.portion_names[0]); //TODO: Make constexpr (together with GetTrackbarName, requires C++20)
  cv::setTrackbarPos(r_trackbar_name, window_name, 255); //Implies cv::imshow with red (R=255, G=0, B=0)
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates how RGB portions can be mixed into different colors." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowImage();
  cv::waitKey(0);
  return 0;
}
