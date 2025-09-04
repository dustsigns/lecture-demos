//Illustration of RGB color mixing
// Andreas Unterweger, 2018-2025
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <algorithm>
#include <memory>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"
#include "window.hpp"

class RGB_data
{
  protected:
    static constexpr char portion_names[] = { 'R', 'G', 'B' };
    static constexpr const auto &first_portion_name = portion_names[0];

    using TrackBarType = imgutils::TrackBar<RGB_data&>;
    std::unique_ptr<TrackBarType> portion_trackbars[comutils::arraysize(portion_names)];
  
    imgutils::Window window;
  
    static cv::Mat GenerateColorImage(const cv::Vec3b &pixel_value)
    {
      constexpr auto image_dimension = 300;
      const cv::Size image_size(image_dimension, image_dimension);
      const cv::Mat image(image_size, CV_8UC3, pixel_value);
      return image;
    }

    static void UpdateImage(RGB_data &data)
    {
      unsigned char RGB_portions[comutils::arraysize(RGB_data::portion_names)]; //RGB portions from trackbar values
      std::transform(std::rbegin(data.portion_trackbars), std::rend(data.portion_trackbars), std::begin(RGB_portions), //BGR order (reverse iteration)
                     [](const std::unique_ptr<TrackBarType> &portion_trackbar)
                       {
                         return static_cast<unsigned char>(portion_trackbar->GetValue());
                       });
      const cv::Vec3b pixel_value(RGB_portions);
      const cv::Mat image = GenerateColorImage(pixel_value);
      data.window.UpdateContent(image);
    }
    
    void AddTrackBars()
    {
      std::transform(std::begin(portion_names), std::end(portion_names), std::begin(portion_trackbars),
                     [this](const char &portion_name)
                           {
                             using namespace std::string_literals;
                             const auto trackbar_name = portion_name + " portion"s;
                             const auto default_value = &portion_name == &first_portion_name ? 255 : 0; //Default red (R=255, G=0, B=0)
                             return std::make_unique<TrackBarType>(trackbar_name, window, 255, 0, default_value, UpdateImage, *this);
                           });
    }


    static constexpr auto window_name = "RGB color mixer";
  public:
    RGB_data() : window(window_name)
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
  RGB_data data;
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates how RGB portions can be mixed into different colors." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowImage();
  return 0;
}
