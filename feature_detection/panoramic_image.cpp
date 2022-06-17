//Illustration of panoramic images
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>

#include "colors.hpp"
#include "combine.hpp"
#include "window.hpp"

static std::vector<cv::Mat> LoadImages(std::vector<const char*> image_paths)
{
  using namespace std::string_literals;
  std::vector<cv::Mat> images(image_paths.size());
  std::transform(image_paths.begin(), image_paths.end(), images.begin(), 
                 [](const char * const filename)
                   {
                     const cv::Mat image = cv::imread(filename);
                     if (image.empty())
                       throw std::runtime_error("Could not read image '"s + filename + "'");
                    return image;
                   });
  return images;
}

static cv::Mat StitchImages(const std::vector<cv::Mat> &images)
{
  assert(images.size() >= 2);
  cv::Mat stitched_image;
  auto stitcher = cv::Stitcher::create();
  const auto status = stitcher->stitch(images, stitched_image);
  if (status != cv::Stitcher::Status::OK)
    throw std::runtime_error("Stitching failed with error " + std::to_string(status));
  return stitched_image;
}

static void ShowImages(const std::vector<cv::Mat> &images)
{
  constexpr auto window_name = "Images combined";
  imgutils::Window window(window_name);
  const cv::Mat original_images = imgutils::CombineImages(images.size(), images.data(), imgutils::CombinationMode::Horizontal);
  const cv::Mat panoramic_image = StitchImages(images);
  const cv::Mat combined_image = imgutils::CombineImages({original_images, panoramic_image}, imgutils::CombinationMode::Vertical);
  window.UpdateContent(combined_image);
  window.ShowInteractive();
}

int main(const int argc, const char * const argv[])
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " <first picture> <second picture> [<third picture> [ ... [<n-th picture>]]]" << std::endl;
    return 1;
  }
  std::vector<const char*> image_paths(argv + 1, argv + argc);
  try
  {
    const auto images = LoadImages(image_paths);
    ShowImages(images);
  }
  catch (const std::runtime_error &e)
  {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}
