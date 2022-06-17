//Illustration of chrominance subsampling
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <memory>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"
#include "window.hpp"

struct color_format
{
  const char * const name; //TODO: Make std::string, also in constructor (requires C++20)
  const int conversion_id;
  const int back_conversion_id;
  
  cv::Mat ConvertImage(const cv::Mat &image, unsigned int &converted_size) const
  {
    cv::Mat converted_image;
    assert(!image.empty());
    cv::cvtColor(image, converted_image, conversion_id); //Convert to subsampled colorspace
    converted_size = converted_image.total() * converted_image.elemSize();
    cv::cvtColor(converted_image, converted_image, back_conversion_id); //Convert back
    return converted_image;
  }
  
  constexpr color_format(const char * const name, const int conversion_id, const int back_conversion_id) 
   : name(name), conversion_id(conversion_id), back_conversion_id(back_conversion_id) { }
};

class subsampling_data
{
  protected: 
    static constexpr const color_format color_formats[] {color_format("4:4:4", cv::COLOR_BGR2YUV, cv::COLOR_YUV2BGR),
                                                         color_format("4:2:0", cv::COLOR_BGR2YUV_I420, cv::COLOR_YUV2BGR_I420),
                                                         /*color_format("4:2:2", cv::COLOR_BGR2YUV_Y422, cv::COLOR_YUV2BGR_Y422)*/ //TODO: Find a conversion ID for BGR to YCbCr 4:2:2 and change order
                                                         color_format("4:0:0", cv::COLOR_BGR2GRAY, cv::COLOR_GRAY2BGR)};
    static constexpr auto &default_color_format = color_formats[1]; //4:2:0 by default
    
    imgutils::Window window;
    
    using RadioButtonType = imgutils::RadioButton<subsampling_data&, const color_format&>;
    std::unique_ptr<RadioButtonType> format_radiobuttons[comutils::arraysize(color_formats)];
    
    const cv::Mat image;
    
    static void UpdateImage(subsampling_data &data, const color_format &format)
    {
      const auto &image = data.image;
      const auto uncompressed_size = image.total() * image.elemSize();
      unsigned int converted_size;
      const cv::Mat converted_image = format.ConvertImage(image, converted_size);
      const cv::Mat combined_image = imgutils::CombineImages({image, converted_image}, imgutils::CombinationMode::Horizontal);
      auto &window = data.window;
      window.UpdateContent(combined_image);
      if (window.IsShown())
      {
        const std::string status_text = "4:4:4 (" + comutils::FormatByte(uncompressed_size) + ") vs. " + format.name + " (" + comutils::FormatByte(converted_size) + ")";
        window.ShowOverlayText(status_text);
      }
    }
    
    void AddRadioButtons()
    {
      std::transform(std::begin(color_formats), std::end(color_formats), std::begin(format_radiobuttons),
                                [this](const color_format &format)
                                      {
                                        const auto radiobutton_name = format.name;
                                        const auto default_checked = &format == &default_color_format;
                                        return std::make_unique<RadioButtonType>(radiobutton_name, window, default_checked, UpdateImage, nullptr, *this, format); //Only process checking, not unchecking (no callback and thus no update)
                                      });
    }
    
    static constexpr auto window_name = "Chrominance subsampling";
  public:    
    subsampling_data(const cv::Mat &image)
     : window(window_name),
       image(image)
    {
      AddRadioButtons();
      UpdateImage(*this, default_color_format); //Update with default values
    }
    
    void ShowImage()
    {
      window.ShowInteractive([this]()
                                   {
                                     UpdateImage(*this, default_color_format); //Update again so that the status bar entries become visible
                                   });
    }
};

static void ShowImage(const cv::Mat &original_image)
{
  subsampling_data data(original_image);
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the effect of chrominance subsampling." << std::endl;
    std::cout << "Usage: " << argv[0] << " <input image>" << std::endl;
    return 1;
  }
  const auto image_filename = argv[1];
  const cv::Mat image = cv::imread(image_filename);
  if (image.empty())
  {
    std::cerr << "Could not read input image '" << image_filename << "'" << std::endl;
    return 2;
  }
  ShowImage(image);
  return 0;
}
