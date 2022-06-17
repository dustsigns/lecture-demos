//Illustration of Gaussian filtering
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "common.hpp"
#include "combine.hpp"
#include "window.hpp"

class Gaussian_data
{
  protected:
    imgutils::Window window;
    
    using TrackBarType = imgutils::TrackBar<Gaussian_data&>;
    TrackBarType sigma_trackbar;
  
    const cv::Mat image;
    
    static void UpdateImage(Gaussian_data &data)
    {
       const cv::Mat &image = data.image;
       const int sigma_percent = data.sigma_trackbar.GetValue();
       const double sigma = sigma_percent / 100.0;
       cv::Mat blurred_image;
       GaussianBlur(image, blurred_image, cv::Size(), sigma);
       const cv::Mat combined_image = imgutils::CombineImages({image, blurred_image}, imgutils::CombinationMode::Horizontal);
       data.window.UpdateContent(combined_image);
    }

    static constexpr auto window_name = "Original vs. blurred";  
    static constexpr auto sigma_trackbar_name = "Sigma [%]";
    
    static constexpr auto min_sigma = 0.01;
    static constexpr auto max_sigma = 20.0;
    static constexpr auto default_sigma = 2.0;
    static_assert(min_sigma <= max_sigma, "The maximum sigma value cannot be greater than the minimum sigma value");
    static_assert(default_sigma <= max_sigma && default_sigma >= min_sigma, "The maximum sigma value cannot be greater than the default sigma value, and the minimum sigma value cannot be smaller than the default sigma value");
  public:
    Gaussian_data(const cv::Mat &image)
     : window(window_name), 
       sigma_trackbar(sigma_trackbar_name, window, static_cast<int>(max_sigma * 100), static_cast<int>(min_sigma * 100), static_cast<int>(default_sigma * 100), UpdateImage, *this), //sigma = 2 (200%) by default
       image(image)
    {
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage(const cv::Mat &image)
{
  Gaussian_data data(image);
  data.ShowImage();
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
  ShowImage(image);
  return 0;
}
