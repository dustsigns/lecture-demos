//Illustration of non-linear lense distortion
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>

#include "combine.hpp"

struct distortion_coefficient
{
  const std::string name;
  const int scaling_factor;
  
  distortion_coefficient(const std::string &name, const double scaling_factor) 
   : name(name), scaling_factor(scaling_factor)
    { }
  
  double GetCoefficientValue(const int value) const
  {
    return value * pow(10, scaling_factor);
  }
};

static constexpr auto number_of_coefficients = 4; //Number of coefficients supported by the undistort function below

struct distortion_data
{
  const cv::Mat image;
  std::array<distortion_coefficient, number_of_coefficients> distortion_coefficients {distortion_coefficient("k1", -7),
                                                                                      distortion_coefficient("k2", -10),
                                                                                      distortion_coefficient("p1", -5),
                                                                                      distortion_coefficient("p2", -5)};
  
  const std::string window_name;
  
  distortion_data(const cv::Mat &image, const std::string &window_name)
   : image(image),
     window_name(window_name) { }
};

static cv::Mat GetStandardCameraMatrix(cv::Size2i image_size)
{
  cv::Mat_<float> camera_matrix = cv::Mat::eye(3, 3, CV_32F); //Focal lengths are 1
  camera_matrix(0, 2) = image_size.width / 2.f; //Principal point is at the center of the image
  camera_matrix(1, 2) = image_size.height / 2.f;
  return camera_matrix;
}

template<size_t... Is>
static constexpr std::array<double, sizeof...(Is)> GetCoefficientArray(const std::array<distortion_coefficient, sizeof...(Is)> &coeffs, const std::array<int, sizeof...(Is)> &values, const std::index_sequence<Is...>&)
{
  return {{ coeffs[Is].GetCoefficientValue(values[Is])... }}; //Continued in this pattern for every available index/coefficient
}

static std::array<double, number_of_coefficients> GetCoefficients(const std::array<distortion_coefficient, number_of_coefficients> &coeffs, const std::array<int, number_of_coefficients> values)
{
  return GetCoefficientArray(coeffs, values, std::make_index_sequence<number_of_coefficients>{}); //Create coefficients by iterating through all (coefficient) array indices
}

static std::string GetTrackbarTitle(const distortion_coefficient &coeff)
{
  return coeff.name + "*10^(" + std::to_string(coeff.scaling_factor) + ")";
}

static std::array<int, number_of_coefficients> GetCoefficientValues(const std::array<distortion_coefficient, number_of_coefficients> &distortion_coefficients, const std::string &window_name)
{
  std::array<int, number_of_coefficients> coefficient_values;
  int i = 0;
  for (auto &coeff : distortion_coefficients)
    coefficient_values[i++] = cv::getTrackbarPos(GetTrackbarTitle(coeff), window_name);
  return coefficient_values;
}

static void ShowDistortedImages(const int, void * const user_data)
{
  auto &data = *(static_cast<distortion_data*>(user_data));
  const cv::Mat &image = data.image;
  const auto coefficient_values = GetCoefficientValues(data.distortion_coefficients, data.window_name);
  const auto distortion_vector = GetCoefficients(data.distortion_coefficients, coefficient_values);
  const auto standard_camera_matrix = GetStandardCameraMatrix(image.size());
  cv::Mat distorted_image;
  cv::undistort(image, distorted_image, standard_camera_matrix, distortion_vector);
  const cv::Mat combined_image = imgutils::CombineImages({image, distorted_image}, imgutils::Horizontal);
  cv::imshow(data.window_name, combined_image);
}

static std::string AddControls(distortion_data &data)
{
  constexpr auto max_negative_value = 100;
  constexpr auto max_positive_value = 100;
  
  for (auto &coeff : data.distortion_coefficients)
  {
    const auto title = GetTrackbarTitle(coeff);
    cv::createTrackbar(title, data.window_name, nullptr, max_negative_value + max_positive_value, ShowDistortedImages, static_cast<void*>(&data));
    cv::setTrackbarMin(title, data.window_name, -max_negative_value);
    cv::setTrackbarMax(title, data.window_name, max_positive_value);
  }
  constexpr auto button_name = "Reset";
  cv::createButton(button_name, [](const int, void * const user_data)
                                  {
                                    auto &data = *(static_cast<distortion_data*>(user_data));
                                    for (auto &coeff : data.distortion_coefficients)
                                    {
                                      const auto title = GetTrackbarTitle(coeff);
                                      cv::setTrackbarPos(title, data.window_name, 0);
                                    }
                                  }, static_cast<void*>(&data), cv::QT_PUSH_BUTTON);
  return GetTrackbarTitle(data.distortion_coefficients[0]);
}

static void ShowImages(const cv::Mat &image)
{
  constexpr auto window_name = "Undistorted vs. distorted";
  cv::namedWindow(window_name);
  cv::moveWindow(window_name, 0, 0);
  static distortion_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  const auto main_parameter_trackbar_name = AddControls(data);
  cv::setTrackbarPos(main_parameter_trackbar_name, window_name, 50); //Implies cv::imshow with 50% of the first coefficient set
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the effect of the distortion vector on a camera image." << std::endl;
    std::cout << "Usage: " << argv[0] << " <camera image>" << std::endl;
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
