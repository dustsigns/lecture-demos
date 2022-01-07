//Illustration of chrominance subsampling
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"

struct color_format
{
  const std::string name;
  const int conversion_id;
  const int back_conversion_id;
  
  color_format(const std::string &name, const int conversion_id, const int back_conversion_id) 
   : name(name), conversion_id(conversion_id), back_conversion_id(back_conversion_id) { }
};

static const color_format color_formats[] {color_format("4:4:4", cv::COLOR_BGR2YUV, cv::COLOR_YUV2BGR),
                                           color_format("4:2:0", cv::COLOR_BGR2YUV_I420, cv::COLOR_YUV2BGR_I420),
                                           /*color_format("4:2:2", cv::COLOR_BGR2YUV_Y422, cv::COLOR_YUV2BGR_Y422)*/ //TODO: Find a conversion ID for BGR to YCbCr 4:2:2 and change order
                                           color_format("4:0:0", cv::COLOR_BGR2GRAY, cv::COLOR_GRAY2BGR)};

const auto &default_color_format = color_formats[1]; //4:2:0 by default

struct subsampling_data
{
  const cv::Mat image;
  
  const std::string window_name;
  
  subsampling_data(const cv::Mat &image, const std::string &window_name) 
   : image(image), window_name(window_name) { }
};

static cv::Mat ConvertImage(const cv::Mat &image, const color_format format, unsigned int &converted_size)
{
  cv::Mat converted_image;
  assert(!image.empty());
  cv::cvtColor(image, converted_image, format.conversion_id); //Convert to subsampled colorspace
  converted_size = converted_image.total() * converted_image.elemSize();
  cv::cvtColor(converted_image, converted_image, format.back_conversion_id); //Convert back
  return converted_image;
}

static void ShowImage(const cv::Mat &original_image)
{
  constexpr auto window_name = "Chrominance subsampling";
  cv::namedWindow(window_name);
  cv::moveWindow(window_name, 0, 0);
  static subsampling_data data(original_image, window_name); //Make variable visible within lambda (cannot capture since it couldn't be converted to a function pointer then)
  for (const auto &format : color_formats)
  {
    cv::createButton(format.name,
                     [](const int state, void * const user_data)
                       {
                          if (!state) //Ignore radio button events where the button becomes unchecked
                           return;
                         auto &format = *(static_cast<const color_format*>(user_data));
                         auto &image = data.image;
                         auto &window_name = data.window_name;
                         const auto uncompressed_size = image.total() * image.elemSize();
                         unsigned int converted_size;
                         const cv::Mat converted_image = ConvertImage(image, format, converted_size);
                         const std::string status_text = "4:4:4 (" + comutils::FormatByte(uncompressed_size) + ") vs. " + format.name + " (" + comutils::FormatByte(converted_size) + ")";
                         cv::displayOverlay(window_name, status_text, 1000);
                         cv::displayStatusBar(window_name, status_text);
                         const cv::Mat combined_image = imgutils::CombineImages({image, converted_image}, imgutils::Horizontal);
                         cv::imshow(window_name, combined_image);
                       }, const_cast<void*>(static_cast<const void*>(&format)), cv::QT_RADIOBOX, &format == &default_color_format); //Make radio button corresponding to default color format checked
  }
  //The first radio button is checked by default, triggering an update event implying cv::imshow
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
  cv::waitKey(0);
  return 0;
}
