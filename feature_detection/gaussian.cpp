//Illustration of Gaussian filtering
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"
#include "combine.hpp"

struct Gaussian_data
{
  const cv::Mat image;
  
  const std::string window_name;
  
  Gaussian_data(const cv::Mat &image, const std::string &window_name)
   : image(image),
     window_name(window_name) { }
};

static const char *AddControls(Gaussian_data &data)
{
  constexpr auto max_sigma = 20;
  constexpr auto scaling_trackbar_name = "Sigma [%]";
  cv::createTrackbar(scaling_trackbar_name, data.window_name, nullptr, max_sigma * 100,
                     [](const int sigma_percent, void * const user_data)
                       {
                         auto &data = *(static_cast<const Gaussian_data*>(user_data));
                         const cv::Mat &image = data.image;
                         const double sigma = sigma_percent / 100.0;
                         cv::Mat blurred_image;
                         GaussianBlur(image, blurred_image, cv::Size(), sigma);
                         const cv::Mat combined_image = imgutils::CombineImages({image, blurred_image}, imgutils::Horizontal);
                         cv::imshow(data.window_name, combined_image);
                       }, static_cast<void*>(&data));
  cv::setTrackbarMin(scaling_trackbar_name, data.window_name, 1);
  return scaling_trackbar_name;
}

static void ShowImages(const cv::Mat &image)
{
  constexpr auto window_name = "Original vs. blurred";
  cv::namedWindow(window_name, cv::WINDOW_GUI_NORMAL | cv::WINDOW_AUTOSIZE);
  cv::moveWindow(window_name, 0, 0);
  static Gaussian_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  const auto main_parameter_trackbar_name = AddControls(data);
  cv::setTrackbarPos(main_parameter_trackbar_name, window_name, 200); //Implies cv::imshow with sigma=2
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the effect of sigma of a Gaussian filter." << std::endl;
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
