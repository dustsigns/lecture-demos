//Illustration of anaglyph images
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "window.hpp"
#include "multiwin.hpp"

static cv::Mat GetAnaglyphImage(const cv::Mat &left_image, const cv::Mat &right_image)
{
  assert(left_image.size() == right_image.size());
  assert(left_image.type() == CV_8UC1);
  assert(right_image.type() == CV_8UC1);
  
  const cv::Mat bgr_channels[3] { right_image, right_image, left_image }; //Set blue channel to right image and red channel to left image; set green channel to blue channel to not leave it empty
  cv::Mat anaglyph_image(left_image.size(), CV_8UC3);
  merge(bgr_channels, 3, anaglyph_image);
  return anaglyph_image;
}

static void ShowImages(const cv::Mat &left_image, const cv::Mat &right_image)
{
  assert(left_image.size() == right_image.size());
  constexpr auto image_window_name = "Left and right images";
  imgutils::Window image_window(image_window_name);
  constexpr auto anaglyph_window_name = "Anaglyph image";
  imgutils::Window anaglyph_window(anaglyph_window_name);
  const cv::Mat combined_image = imgutils::CombineImages({left_image, right_image}, imgutils::CombinationMode::Horizontal);
  image_window.UpdateContent(combined_image);
  const cv::Mat anaglyph_image = GetAnaglyphImage(left_image, right_image);
  anaglyph_window.UpdateContent(anaglyph_image);
  imgutils::MultiWindow all_windows({&image_window, &anaglyph_window}, imgutils::WindowAlignment::Horizontal);
  all_windows.ShowInteractive();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    std::cout << "Illustrates stereoscopy with anaglyph images." << std::endl;
    std::cout << "Usage: " << argv[0] << " <left image> <right image>" << std::endl;
    return 1;
  }
  const auto left_image_filename = argv[1];
  const cv::Mat left_image = cv::imread(left_image_filename, cv::IMREAD_GRAYSCALE);
  if (left_image.empty())
  {
    std::cerr << "Could not read left image '" << left_image_filename << "'" << std::endl;
    return 2;
  }
  const auto right_image_filename = argv[2];
  const cv::Mat right_image = cv::imread(right_image_filename, cv::IMREAD_GRAYSCALE);
  if (right_image.empty())
  {
    std::cerr << "Could not read right image '" << right_image_filename << "'" << std::endl;
    return 3;
  }
  ShowImages(left_image, right_image);
  return 0;
}
