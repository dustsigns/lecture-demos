//Illustration of downsampling and upsampling
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <utility>
#include <memory>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"
#include "combine.hpp"
#include "window.hpp"

class resampling_data
{
  protected:
    using resampling_algorithm = std::pair<const char*, int>;
    static constexpr resampling_algorithm resampling_algorithms[] {std::make_pair("Nearest neighbor", cv::INTER_NEAREST),
                                                                   std::make_pair("Bilinear", cv::INTER_LINEAR),
                                                                   std::make_pair("Lanczos-4", cv::INTER_LANCZOS4)};
    static constexpr auto &default_algorithm = resampling_algorithms[0]; //Nearest neighbor by default

    imgutils::Window window;
  
    using RadioButtonType = imgutils::RadioButton<resampling_data&, const int>;
    std::unique_ptr<RadioButtonType> algorithm_radiobuttons[comutils::arraysize(resampling_algorithms)];
    
    using TrackBarType = imgutils::TrackBar<resampling_data&>;
    TrackBarType scaling_factor_trackbar;
  
    const cv::Mat image;
    int resampling_algorithm_id; //The current algorithm needs to be stored as there is no reliable way to determine the currently checked radio button
  
    static void UpdateImage(resampling_data &data)
    {
      const auto scaling_factor_percent = data.scaling_factor_trackbar.GetValue();
      const cv::Mat &image = data.image;
      const double scaling_factor = sqrt(scaling_factor_percent / 100.0);
      cv::Mat downsampled_image;
      cv::resize(image, downsampled_image, cv::Size(), scaling_factor, scaling_factor, data.resampling_algorithm_id);
      cv::Mat upsampled_image;
      cv::resize(downsampled_image, upsampled_image, image.size(), 0, 0, data.resampling_algorithm_id);
      const cv::Mat combined_image = imgutils::CombineImages({image, upsampled_image, downsampled_image}, imgutils::CombinationMode::Horizontal);
      data.window.UpdateContent(combined_image);
    }
    
    static void UpdateResamplingAlgorithm(resampling_data &data, const int algorithm_id)
    {
      data.resampling_algorithm_id = algorithm_id;
      UpdateImage(data);
    }
  
    void AddRadioButtons()
    {
      std::transform(std::begin(resampling_algorithms), std::end(resampling_algorithms), std::begin(algorithm_radiobuttons),
                                [this](const resampling_algorithm &algorithm)
                                      {
                                        const auto radiobutton_name = algorithm.first;
                                        const auto algorithm_id = algorithm.second;
                                        const auto default_checked = &algorithm == &default_algorithm;
                                        return std::make_unique<RadioButtonType>(radiobutton_name, window, default_checked, UpdateResamplingAlgorithm, nullptr, *this, algorithm_id); //Only process checking, not unchecking (no callback and thus no update)
                                      });
    }
  
    static constexpr auto window_name = "Original vs. resampled (incl. intermediate downsampled)";
    static constexpr auto scaling_factor_trackbar_name = "Scaling [%]";
  public:
    resampling_data(const cv::Mat &image)
     : window(window_name),
       scaling_factor_trackbar(scaling_factor_trackbar_name, window, 100, 1, 50, UpdateImage, *this), //50% scaling factor by default, minimum 1
       image(image),
       resampling_algorithm_id(default_algorithm.second)
    {
      AddRadioButtons();
      UpdateImage(*this); //Update with default values
    }
       
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage(const cv::Mat &image)
{
  resampling_data data(image);
  data.ShowImage();
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
  ShowImage(image);
  return 0;
}
