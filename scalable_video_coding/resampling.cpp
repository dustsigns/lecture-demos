//Illustration of downsampling and upsampling
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"
#include "combine.hpp"

struct resampling_data
{
  const cv::Mat image;
  int resampling_algorithm;
  
  const std::string window_name;
  
  static constexpr auto scaling_trackbar_name = "Scaling [%]";
  
  resampling_data(const cv::Mat &image, const std::string &window_name)
   : image(image),
     resampling_algorithm(cv::INTER_NEAREST), window_name(window_name) { }
};

constexpr std::pair<const char*, int> resampling_algorithms[] {std::make_pair("Nearest neighbor", cv::INTER_NEAREST),
                                                               std::make_pair("Bilinear", cv::INTER_LINEAR),
                                                               std::make_pair("Lanczos-4", cv::INTER_LANCZOS4)};

static void ShowResampledImages(const int scaling_factor_percent, void * const user_data)
{
  auto &data = *(static_cast<const resampling_data*>(user_data));
  const cv::Mat &image = data.image;
  const double scaling_factor = sqrt(scaling_factor_percent / 100.0);
  cv::Mat downsampled_image;
  cv::resize(image, downsampled_image, cv::Size(), scaling_factor, scaling_factor, data.resampling_algorithm);
  cv::Mat upsampled_image;
  cv::resize(downsampled_image, upsampled_image, image.size(), 0, 0, data.resampling_algorithm);
  const cv::Mat combined_image = imgutils::CombineImages({image, upsampled_image, downsampled_image}, imgutils::Horizontal);
  cv::imshow(data.window_name, combined_image);
}

template<int A>
static void SetResamplingAlgorithm(const int state, void * const user_data)
{
  if (!state) //Ignore radio button events where the button becomes unchecked
    return;
  auto &data = *(static_cast<resampling_data*>(user_data));
  data.resampling_algorithm = A;
  const int scaling_factor_percent = cv::getTrackbarPos(data.scaling_trackbar_name, data.window_name);
  ShowResampledImages(scaling_factor_percent, user_data);
}

template<size_t... Is>
static void CreateButtons(void * const data, const std::index_sequence<Is...>&)
{
  constexpr auto index_limit = comutils::arraysize(resampling_algorithms);
  static_assert(sizeof...(Is) <= index_limit, "Number of array indices is out of bounds");
  ((void)cv::createButton(resampling_algorithms[Is].first, SetResamplingAlgorithm<resampling_algorithms[Is].second>, data, cv::QT_RADIOBOX, Is == 0), ...); //Make first radio button checked
}

static void CreateAllButtons(void * const data)
{
  constexpr auto N = comutils::arraysize(resampling_algorithms);
  CreateButtons(data, std::make_index_sequence<N>{}); //Create N buttons with callbacks for every index
}

static void AddControls(resampling_data &data)
{
  cv::createTrackbar(data.scaling_trackbar_name, data.window_name, nullptr, 100, ShowResampledImages, static_cast<void*>(&data));
  cv::setTrackbarMin(data.scaling_trackbar_name, data.window_name, 1);
  CreateAllButtons(static_cast<void*>(&data));
}

static void ShowImages(const cv::Mat &image)
{
  constexpr auto window_name = "Original vs. resampled (incl. intermediate downsampled)";
  cv::namedWindow(window_name);
  cv::moveWindow(window_name, 0, 0);
  static resampling_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  AddControls(data);
  cv::setTrackbarPos(data.scaling_trackbar_name, window_name, 50); //Implies cv::imshow with 50% scaling factor
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the effect of resampling." << std::endl;
    std::cout << "Usage: " << argv[0] << " <input image>" << std::endl;
    return 1;
  }
  const auto filename = argv[1];
  const cv::Mat image = cv::imread(filename);
  if (image.empty())
  {
    std::cerr << "Could not read input image '" << filename << "'" << std::endl;
    return 2;
  }
  ShowImages(image);
  cv::waitKey(0);
  return 0;
}
