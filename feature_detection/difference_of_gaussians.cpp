//Illustration of DoG computation
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"
#include "imgmath.hpp"

struct DoG_data
{
  const cv::Mat image;
  
  const std::string image_window_name;
  const std::string difference_window_name;
  
  static constexpr auto sigma_trackbar_name = "Sigma [%]";
  static constexpr auto k_trackbar_name = "k [%]";
  
  bool initialized;
  
  DoG_data(const cv::Mat &image, const std::string &image_window_name, const std::string &difference_window_name)
   : image(image),
     image_window_name(image_window_name), difference_window_name(difference_window_name),
     initialized(false) { }
};

static void ShowDifferenceImage(const std::string &window_name, const cv::Mat &first_image, const cv::Mat &second_image)
{
  assert(first_image.type() == CV_8UC1);
  assert(first_image.type() == CV_8UC1);
  const cv::Mat difference_image = imgutils::SubtractImages(first_image, second_image);
  cv::imshow(window_name, imgutils::ConvertDifferenceImage(difference_image));
}

static void UpdateImages(const int, void * const user_data)
{
  auto &data = *(static_cast<const DoG_data*>(user_data));
  if (!data.initialized)
    return;
  const cv::Mat &image = data.image;
  const int sigma_percent = cv::getTrackbarPos(data.sigma_trackbar_name, data.image_window_name);
  const int k_percent = cv::getTrackbarPos(data.k_trackbar_name, data.image_window_name);
  const double sigma = sigma_percent / 100.0;
  const double k = k_percent / 100.0;
  cv::Mat blurred_image_sigma, blurred_image_k_times_sigma;
  GaussianBlur(image, blurred_image_sigma, cv::Size(), sigma);
  GaussianBlur(image, blurred_image_k_times_sigma, cv::Size(), k * sigma);
  const cv::Mat combined_image = imgutils::CombineImages({blurred_image_sigma, blurred_image_k_times_sigma}, imgutils::CombinationMode::Horizontal);
  cv::imshow(data.image_window_name, combined_image);
  ShowDifferenceImage(data.difference_window_name, blurred_image_k_times_sigma, blurred_image_sigma);
}

static void ShowImages(const cv::Mat &image)
{
  constexpr auto max_sigma = 5.0;
  constexpr auto max_k = 5;
  
  constexpr auto image_window_name = "Blurred (sigma), blurred (k * sigma)";
  cv::namedWindow(image_window_name, cv::WINDOW_GUI_NORMAL | cv::WINDOW_AUTOSIZE);
  cv::moveWindow(image_window_name, 0, 0);
  constexpr auto difference_window_name = "Blurred(k * sigma) - blurred (sigma)";
  cv::namedWindow(difference_window_name, cv::WINDOW_GUI_NORMAL | cv::WINDOW_AUTOSIZE);
  cv::moveWindow(difference_window_name, 2 * image.cols + 3 + 3, 0); //Move difference window right beside the comparison window (2 images plus 3 border pixels plus additional distance)
  static DoG_data data(image, image_window_name, difference_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  cv::createTrackbar(data.sigma_trackbar_name, data.image_window_name, nullptr, max_sigma * 100, UpdateImages, static_cast<void*>(&data));
  cv::setTrackbarMin(data.sigma_trackbar_name, data.image_window_name, 1);
  cv::createTrackbar(data.k_trackbar_name, data.image_window_name, nullptr, max_k * 100, UpdateImages, static_cast<void*>(&data));
  cv::setTrackbarMin(data.k_trackbar_name, data.image_window_name, 1);
  cv::setTrackbarPos(data.sigma_trackbar_name, image_window_name, 200); //Implies setting sigma=2 without cv::imshow
  data.initialized = true;
  cv::setTrackbarPos(data.k_trackbar_name, image_window_name, 150); //Implies cv::imshow with k=1.5
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the effect of the differences in sigma for the DoG." << std::endl;
    std::cout << "Usage: " << argv[0] << " <input image>" << std::endl;
    return 1;
  }
  const auto image_filename = argv[1];
  const cv::Mat image = cv::imread(image_filename, cv::IMREAD_GRAYSCALE);
  if (image.empty())
  {
    std::cerr << "Could not read input image '" << image_filename << "'" << std::endl;
    return 2;
  }
  ShowImages(image);
  cv::waitKey(0);
  return 0;
}
