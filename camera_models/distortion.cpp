//Illustration of non-linear lense distortion
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>

#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

struct distortion_coefficient
{
  const string name;
  const int scaling_factor;
  
  int value;
  
  distortion_coefficient(const string &name, const double scaling_factor) 
   : name(name), scaling_factor(scaling_factor),
     value(0) { }
  
  double GetCoefficientValue() const
  {
    return value * pow(10, scaling_factor);
  }
};

static constexpr auto number_of_coefficients = 4; //Number of coefficients supported by the undistort function below

struct distortion_data
{
  const Mat image;
  array<distortion_coefficient, number_of_coefficients> distortion_coefficients {distortion_coefficient("k1", -7),
                                                                                 distortion_coefficient("k2", -10),
                                                                                 distortion_coefficient("p1", -5),
                                                                                 distortion_coefficient("p2", -5)};
  
  const string window_name;
  
  distortion_data(const Mat &image, const string &window_name)
   : image(image),
     window_name(window_name) { }
};

static Mat GetStandardCameraMatrix(Size2i image_size)
{
  Mat_<float> camera_matrix = Mat::eye(3, 3, CV_32F); //Focal lengths are 1
  camera_matrix(0, 2) = image_size.width / 2.f; //Principal point is at the center of the image
  camera_matrix(1, 2) = image_size.height / 2.f;
  return camera_matrix;
}

template<size_t... Is>
static constexpr array<double, sizeof...(Is)> GetCoefficientArray(const array<distortion_coefficient, sizeof...(Is)> &coeffs, const index_sequence<Is...>&)
{
  return {{ coeffs[Is].GetCoefficientValue()... }}; //Continued in this pattern for every available index/coefficient
}

static array<double, number_of_coefficients> GetCoefficients(const array<distortion_coefficient, number_of_coefficients> &coeffs)
{
  return GetCoefficientArray(coeffs, make_index_sequence<number_of_coefficients>{}); //Create coefficients by iterating through all (coefficient) array indices
}

static void ShowDistortedImages(const int, void * const user_data)
{
  auto &data = *((const distortion_data * const)user_data);
  const Mat &image = data.image;
  const auto distortion_vector = GetCoefficients(data.distortion_coefficients);
  const auto standard_camera_matrix = GetStandardCameraMatrix(image.size());
  Mat distorted_image;
  undistort(image, distorted_image, standard_camera_matrix, distortion_vector);
  const Mat combined_image = CombineImages({image, distorted_image}, Horizontal);
  imshow(data.window_name, combined_image);
}

static string GetTrackbarTitle(const distortion_coefficient &coeff)
{
  return coeff.name + "*10^(" + to_string(coeff.scaling_factor) + ")";
}

static string AddControls(distortion_data &data)
{
  constexpr auto max_negative_value = 100;
  constexpr auto max_positive_value = 100;
  
  for (auto &coeff : data.distortion_coefficients)
  {
    const auto title = GetTrackbarTitle(coeff);
    createTrackbar(title, data.window_name, &coeff.value, max_negative_value + max_positive_value, ShowDistortedImages, (void*)&data);
    setTrackbarMin(title, data.window_name, -max_negative_value);
    setTrackbarMax(title, data.window_name, max_positive_value);
  }
  createButton("Reset", [](const int, void * const user_data)
                          {
                            auto &data = *((distortion_data * const)user_data);
                            for (auto &coeff : data.distortion_coefficients)
                            {
                              const auto title = GetTrackbarTitle(coeff);
                              setTrackbarPos(title, data.window_name, 0);
                            }
                          }, (void*)&data, QT_PUSH_BUTTON);
  return GetTrackbarTitle(data.distortion_coefficients[0]);
}

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "Undistorted vs. distorted";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  static distortion_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  const auto main_parameter_trackbar_name = AddControls(data);
  setTrackbarPos(main_parameter_trackbar_name, window_name, 50); //Implies imshow with 50% of the first coefficient set
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the effect of the distortion vector on a camera image." << endl;
    cout << "Usage: " << argv[0] << " <camera image>" << endl;
    return 1;
  }
  const auto filename = argv[1];
  const Mat image = imread(filename);
  if (image.empty())
  {
    cerr << "Could not read input image '" << filename << "'" << endl;
    return 2;
  }
  ShowImages(image);
  waitKey(0);
  return 0;
}
