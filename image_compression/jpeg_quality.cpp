//Illustration of JPEG quality levels
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"
#include "imgmath.hpp"

struct JPEG_data
{
  const cv::Mat image;
  
  const std::string image_window_name;
  const std::string difference_window_name;
  
  JPEG_data(const cv::Mat &image, const std::string &image_window_name, const std::string &difference_window_name)
   : image(image),
     image_window_name(image_window_name), difference_window_name(difference_window_name) { }
};

static cv::Mat CompressImage(const cv::Mat &image, const unsigned char quality, unsigned int &compressed_size)
{
  assert(quality <= 100);
  std::vector<uchar> compressed_bits;
  assert(imencode(".jpg", image, compressed_bits, std::vector<int>({cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, quality, cv::ImwriteFlags::IMWRITE_JPEG_OPTIMIZE, 1})));
  compressed_size = compressed_bits.size();
  const cv::Mat compressed_image = cv::imdecode(compressed_bits, cv::ImreadModes::IMREAD_COLOR);
  assert(!compressed_image.empty());
  return compressed_image;
}

static cv::Mat GetYChannelFromRGBImage(const cv::Mat &image)
{
  cv::Mat gray;
  cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY); //Identical to cv::COLOR_BGR2YCrCb with just the luma channel (see https://docs.opencv.org/master/de/d25/imgproc_color_conversions.html#color_convert_rgb_ycrcb)
  return gray;
}

static void ShowDifferenceImage(const std::string &window_name, const cv::Mat &image, const cv::Mat &compressed_image)
{
  const cv::Mat image_y = GetYChannelFromRGBImage(image);
  const cv::Mat compressed_image_y = GetYChannelFromRGBImage(compressed_image);
  const cv::Mat difference_y = imgutils::SubtractImages(compressed_image_y, image_y);
  const double YPSNR = imgutils::PSNR(imgutils::MSE(difference_y));
  const std::string status_text = "Y-PSNR: " + comutils::FormatLevel(YPSNR);
  cv::displayStatusBar(window_name, status_text);
  cv::displayOverlay(window_name, status_text, 1000);
  cv::imshow(window_name, imgutils::ConvertDifferenceImage(difference_y));
}

static void ShowImages(const cv::Mat &image)
{
  constexpr auto image_window_name = "Uncompressed vs. JPEG compressed";
  cv::namedWindow(image_window_name);
  cv::moveWindow(image_window_name, 0, 0);
  constexpr auto difference_window_name = "Difference";
  cv::namedWindow(difference_window_name);
  cv::moveWindow(difference_window_name, image.cols + 3, image.rows + 125); //Move difference window right below the right part of the comparison window (horizontal: image size plus 3 border pixels plus additional distance, vertial: image size plus additional distance for track bars etc.) //TODO: Get windows positions and calculate proper offsets
  static JPEG_data data(image, image_window_name, difference_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  constexpr auto trackbar_name = "Quality";
  cv::createTrackbar(trackbar_name, image_window_name, nullptr, 100,
                     [](const int quality, void * const user_data)
                       {
                         auto &data = *(static_cast<const JPEG_data*>(user_data));
                         const cv::Mat &image = data.image;
                         const auto uncompressed_size = image.total() * image.elemSize();
                         unsigned int compressed_size;
                         cv::Mat compressed_image = CompressImage(image, quality, compressed_size);
                         const std::string status_text = comutils::FormatByte(uncompressed_size) + " vs. " + comutils::FormatByte(compressed_size);
                         cv::displayOverlay(data.image_window_name, status_text, 1000);
                         cv::displayStatusBar(data.image_window_name, status_text);
                         const cv::Mat combined_image = imgutils::CombineImages({image, compressed_image}, imgutils::Horizontal);
                         cv::imshow(data.image_window_name, combined_image);
                         ShowDifferenceImage(data.difference_window_name, image, compressed_image);
                       }, static_cast<void*>(&data));
  cv::setTrackbarPos(trackbar_name, image_window_name, 50); //Implies cv::imshow with 50%
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates JPEG compression at different quality levels." << std::endl;
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
  ShowImages(image);
  cv::waitKey(0);
  return 0;
}
