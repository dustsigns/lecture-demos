//Illustration of DoG computation
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"
#include "imgmath.hpp"
#include "window.hpp"
#include "multiwin.hpp"

class DoG_data
{
  protected:
    imgutils::Window image_window;
      
    using TrackBarType = imgutils::TrackBar<DoG_data&>;
    TrackBarType sigma_trackbar;
    TrackBarType k_trackbar;
    
    imgutils::Window difference_window;
    
    imgutils::MultiWindow all_windows;
    
    const cv::Mat image;

    void UpdateDifferenceImage(const cv::Mat &first_image, const cv::Mat &second_image)
    {
      assert(first_image.type() == CV_8UC1);
      assert(first_image.type() == CV_8UC1);
      const cv::Mat difference_image = imgutils::SubtractImages(first_image, second_image);
      const cv::Mat converted_difference_image = imgutils::ConvertDifferenceImage(difference_image);
      difference_window.UpdateContent(converted_difference_image);
    }
    
    static void UpdateImages(DoG_data &data)
    {
      const cv::Mat &image = data.image;
      const int sigma_percent = data.sigma_trackbar.GetValue();
      const int k_percent = data.k_trackbar.GetValue();
      const double sigma = sigma_percent / 100.0;
      const double k = k_percent / 100.0;
      cv::Mat blurred_image_sigma, blurred_image_k_times_sigma;
      GaussianBlur(image, blurred_image_sigma, cv::Size(), sigma);
      GaussianBlur(image, blurred_image_k_times_sigma, cv::Size(), k * sigma);
      const cv::Mat combined_image = imgutils::CombineImages({blurred_image_sigma, blurred_image_k_times_sigma}, imgutils::CombinationMode::Horizontal);
      data.image_window.UpdateContent(combined_image);
      data.UpdateDifferenceImage(blurred_image_k_times_sigma, blurred_image_sigma);
    }

    static constexpr auto image_window_name = "Blurred(sigma), blurred(k * sigma)";
    static constexpr auto sigma_trackbar_name = "Sigma [%]";
    static constexpr auto k_trackbar_name = "k [%]";
    
    static constexpr auto min_sigma = 1.0;
    static constexpr auto max_sigma = 5.0;
    static constexpr auto default_sigma = 2.0;
    static_assert(min_sigma <= max_sigma, "The maximum sigma value cannot be greater than the minimum sigma value");
    static_assert(default_sigma <= max_sigma && default_sigma >= min_sigma, "The maximum sigma value cannot be greater than the default sigma value, and the minimum sigma value cannot be smaller than the default sigma value");
    
    static constexpr auto min_k = 1.0;
    static constexpr auto max_k = 5.0;
    static constexpr auto default_k = 1.5;
    static_assert(min_k <= max_k, "The maximum k value cannot be greater than the minimum k value");
    static_assert(default_k <= max_k && default_k >= min_k, "The maximum k value cannot be greater than the default k value, and the minimum k value cannot be smaller than the default k value");
    
    static constexpr auto difference_window_name = "Blurred(k * sigma) - blurred(sigma)";
  public:    
    DoG_data(const cv::Mat &image)
     : image_window(image_window_name),
       sigma_trackbar(sigma_trackbar_name, image_window, static_cast<int>(max_sigma * 100), static_cast<int>(min_sigma * 100), static_cast<int>(default_sigma * 100), UpdateImages, *this), //sigma = 2 (200%) by default
       k_trackbar(k_trackbar_name, image_window, static_cast<int>(max_k * 100), static_cast<int>(min_k * 100), static_cast<int>(default_k * 100), UpdateImages, *this), //k = 1.5 (150%) by default
       difference_window(difference_window_name),
       all_windows({&image_window, &difference_window}, imgutils::WindowAlignment::Horizontal),
       image(image)
    {
      UpdateImages(*this); //Update with default values
    }
    
    void ShowImages()
    {
      all_windows.ShowInteractive();
    }
};

static void ShowImages(const cv::Mat &image)
{
  DoG_data data(image);
  data.ShowImages();
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
  return 0;
}
