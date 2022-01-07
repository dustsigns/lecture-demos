//Illustration of RGB and YCbCr component decomposition
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"

static void ShowImages(const cv::Mat &image)
{
  constexpr auto window_name = "RGB vs. YCbCr";
  cv::namedWindow(window_name);
  cv::moveWindow(window_name, 0, 0);
  cv::Mat bgr_planes[3];
  cv::split(image, bgr_planes);
  cv::Mat ycrcb_image;
  cv::cvtColor(image, ycrcb_image, cv::COLOR_BGR2YCrCb); //TODO: Allow subsampling (how? cv::cvtColor makes a 1-channel matrix with cv::COLOR_BGR2YUV_*)
  cv::Mat ycrcb_planes[3];
  cv::split(ycrcb_image, ycrcb_planes);
  const cv::Mat rgb_planes_combined = imgutils::CombineImages({image, bgr_planes[2], bgr_planes[1], bgr_planes[0]}, imgutils::CombinationMode::Horizontal); //BGR as RGB
  const cv::Mat ycbcr_planes_combined = imgutils::CombineImages({image, ycrcb_planes[0], ycrcb_planes[2], ycrcb_planes[1]}, imgutils::CombinationMode::Horizontal); //YCrCb as YCbCr
  cv::Mat combined_images = imgutils::CombineImages({rgb_planes_combined, ycbcr_planes_combined}, imgutils::CombinationMode::Vertical);
  cv::resize(combined_images, combined_images, cv::Size(), 1 / sqrt(2), 1 / sqrt(2), cv::INTER_LANCZOS4); //TODO: Don't resize, but find another way to fit the window(s) to the screen size, e.g., by allowing to hide the original image via a checkbox
  cv::imshow(window_name, combined_images);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Extracts the RGB and YCbCr channels of an image and displays them." << std::endl;
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
  ShowImages(image);
  cv::waitKey(0);
  return 0;
}
