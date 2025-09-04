//Illustration of YCbCr color mixing
// Andreas Unterweger, 2018-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <algorithm>
#include <memory>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "common.hpp"
#include "window.hpp"

class YCbCr_data
{
  protected:
    static constexpr const char * const portion_names[] = { "Y", "Cb", "Cr" }; //TODO: Make constexpr std::string (requires C++20)
    static constexpr const auto &first_portion_name = portion_names[0];
  
    using TrackBarType = imgutils::TrackBar<YCbCr_data&>;
    std::unique_ptr<TrackBarType> portion_trackbars[comutils::arraysize(portion_names)];
  
    imgutils::Window window;
  
    static cv::Mat GenerateColorImage(const cv::Vec3b &pixel_value)
    {
      constexpr auto image_dimension = 300;
      const cv::Size image_size(image_dimension, image_dimension);
      const cv::Mat image(image_size, CV_8UC3, pixel_value);
      return image;
    }
    
    static void UpdateImage(YCbCr_data &data)
    {
      unsigned char YCbCr_portions[comutils::arraysize(YCbCr_data::portion_names)]; //YCbCr portions from trackbar values
      std::transform(std::begin(data.portion_trackbars), std::end(data.portion_trackbars), std::begin(YCbCr_portions),
                     [](const std::unique_ptr<TrackBarType> &portion_trackbar)
                       {
                         return static_cast<unsigned char>(portion_trackbar->GetValue());
                       });
      const cv::Vec3b pixel_value(YCbCr_portions[0], YCbCr_portions[2], YCbCr_portions[1]); //YCrCb order
      const cv::Mat ycrcb_image = GenerateColorImage(pixel_value);
      cv::Mat rgb_image;
      cv::cvtColor(ycrcb_image, rgb_image, cv::COLOR_YCrCb2BGR);
      data.window.UpdateContent(rgb_image);
    }
    
    void AddTrackBars()
    {
      std::transform(std::begin(portion_names), std::end(portion_names), std::begin(portion_trackbars),
                     [this](const char * const &portion_name)
                           {
                             using namespace std::string_literals;
                             const auto trackbar_name = portion_name + " portion"s;
                             const auto default_value = &portion_name == &first_portion_name ? 255 : 0; //Default red (Y=255, G=0, B=0)
                             return std::make_unique<TrackBarType>(trackbar_name, window, 255, 0, default_value, UpdateImage, *this);
                           });
    }
  
    static constexpr auto window_name = "YCbCr color mixer";  
  public:
    YCbCr_data() : window(window_name)
    {
      AddTrackBars();
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage()
{
  YCbCr_data data;
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates how YCbCr portions can be mixed into different colors." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowImage();
  return 0;
}
