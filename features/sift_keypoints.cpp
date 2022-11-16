//Illustration of SIFT keypoints
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>

#include "combine.hpp"
#include "window.hpp"

static void DetectFeatures(const cv::Mat &image, std::vector<cv::KeyPoint> &keypoints)
{
  auto feature_detector = cv::SIFT::create();
  feature_detector->detect(image, keypoints);
}

static cv::Mat DrawKeypoints(const cv::Mat &image, const std::vector<cv::KeyPoint> &keypoints)
{
  cv::Mat image_with_keypoints;
  drawKeypoints(image, keypoints, image_with_keypoints, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
  return image_with_keypoints;
}

static cv::Mat VisualizeKeypoints(const cv::Mat &image)
{
  std::vector<cv::KeyPoint> keypoints;
  DetectFeatures(image, keypoints);
  const cv::Mat image_with_keypoints = DrawKeypoints(image, keypoints);
  return image_with_keypoints;
}

static void ShowImage(const cv::Mat &image)
{
  constexpr auto window_name = "Image without and with keypoints";
  imgutils::Window window(window_name);
  const cv::Mat image_with_keypoints = VisualizeKeypoints(image);
  const cv::Mat combined_image = imgutils::CombineImages({image, image_with_keypoints}, imgutils::CombinationMode::Horizontal);
  window.UpdateContent(combined_image);
  window.ShowInteractive();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the SIFT keypoints of an image" << std::endl;
    std::cout << "Usage: " << argv[0] << " <input image>" << std::endl;
    return 1;
  }
  const auto image_filename = argv[1];
  const cv::Mat image = cv::imread(image_filename);
  if (image.empty())
  {
    std::cerr << "Could not read input image '" << image_filename << "'" << std::endl;
    return 2;
  }
  ShowImage(image);
  return 0;
}
