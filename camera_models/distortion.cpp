//Illustration of non-linear lense distortion
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <array>
#include <cassert>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>

#include "combine.hpp"
#include "window.hpp"

struct distortion_coefficient
{
  const char * const name; //TODO: Convert to std::string (requires C++20)
  const int scaling_factor;
  
  constexpr distortion_coefficient(const char * const &name, const double scaling_factor) 
   : name(name), scaling_factor(scaling_factor)
    { }
  
  double GetCoefficientValue(const int value) const
  {
    return value * pow(10, scaling_factor);
  }
};

class distortion_data
{
  protected:
    static constexpr distortion_coefficient distortion_coefficients[] { distortion_coefficient("k1", -7),
                                                                        distortion_coefficient("k2", -10),
                                                                        distortion_coefficient("p1", -5),
                                                                        distortion_coefficient("p2", -5) };
    static constexpr const auto &first_distortion_coefficient = distortion_coefficients[0];
    static constexpr auto number_of_coefficients = comutils::arraysize(distortion_coefficients);

    imgutils::Window window;
    
    using TrackBarType = imgutils::TrackBar<distortion_data&>;
    std::unique_ptr<TrackBarType> coefficient_trackbars[number_of_coefficients];
    
    using ButtonType = imgutils::Button<distortion_data&>;
    std::unique_ptr<ButtonType> reset_button;

    const cv::Mat image;
  
    static std::string GetTrackbarName(const distortion_coefficient &coeff)
    {
      using namespace std::string_literals;
      return coeff.name + "*10^("s + std::to_string(coeff.scaling_factor) + ")";
    }

    std::array<double, number_of_coefficients> GetCoefficientValues()
    {
      assert(comutils::arraysize(distortion_coefficients) == comutils::arraysize(coefficient_trackbars) && comutils::arraysize(distortion_coefficients) == number_of_coefficients);
      std::array<double, number_of_coefficients> coefficient_values;
      for (size_t i = 0; i < number_of_coefficients; i++)
      {
        const auto value = coefficient_trackbars[i]->GetValue();
        coefficient_values[i] = distortion_coefficients[i].GetCoefficientValue(value);
      }
      return coefficient_values;
    }

    cv::Mat GetStandardCameraMatrix()
    {
      const auto image_size = image.size();
      cv::Mat_<float> camera_matrix = cv::Mat::eye(3, 3, CV_32F); //Focal lengths are 1
      camera_matrix(0, 2) = image_size.width / 2.f; //Principal point is at the center of the image
      camera_matrix(1, 2) = image_size.height / 2.f;
      return camera_matrix;
    }

    static void UpdateImage(distortion_data &data)
    {
      const cv::Mat &image = data.image;
      const auto distortion_vector = data.GetCoefficientValues();
      const auto standard_camera_matrix = data.GetStandardCameraMatrix();
      cv::Mat distorted_image;
      cv::undistort(image, distorted_image, standard_camera_matrix, distortion_vector);
      const cv::Mat combined_image = imgutils::CombineImages({image, distorted_image}, imgutils::CombinationMode::Horizontal);
      data.window.UpdateContent(combined_image);
    }
    
    static void ResetCoefficients(distortion_data &data)
    {
      for (const auto &trackbar : data.coefficient_trackbars)
        trackbar->SetValue(0);
      UpdateImage(data);
    }
  
    void AddControls()
    {
      std::transform(std::begin(distortion_coefficients), std::end(distortion_coefficients), std::begin(coefficient_trackbars),
                     [this](const distortion_coefficient &coefficient)
                           {
                                 constexpr auto max_negative_value = 100;
                                 constexpr auto max_positive_value = 100;
                                 const auto trackbar_name = GetTrackbarName(coefficient);
                                 const auto default_value = &coefficient == &first_distortion_coefficient ? 50 : 0; //Distortion of 50
                                 return std::make_unique<TrackBarType>(trackbar_name, window, max_positive_value, -max_negative_value, default_value, UpdateImage, *this);
                           });
      reset_button = std::make_unique<ButtonType>(reset_button_name, window, ResetCoefficients, *this);
    }
  
    static constexpr auto window_name = "Undistorted vs. distorted";
    static constexpr auto reset_button_name = "Reset";
  public:    
    distortion_data(const cv::Mat &image)
     : window(window_name),
       image(image)
    { 
      AddControls();
      UpdateImage(*this);
    }
    
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage(const cv::Mat &image)
{
  distortion_data data(image);
  data.ShowImage();
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
  ShowImage(image);
  return 0;
}
