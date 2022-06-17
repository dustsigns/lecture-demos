//Illustration of JPEG quality levels
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"
#include "imgmath.hpp"
#include "window.hpp"
#include "multiwin.hpp"

class JPEG_data
{
  protected:
    imgutils::Window image_window;
    
    using TrackBarType = imgutils::TrackBar<JPEG_data&>;
    TrackBarType quality_trackbar;
    
    imgutils::Window difference_window;
    
    imgutils::MultiWindow all_windows;
  
    const cv::Mat image;
    
    cv::Mat CompressImage(const unsigned char quality, unsigned int &compressed_size)
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
    
    void UpdateDifferenceImage(const cv::Mat &compressed_image)
    {
      const cv::Mat image_y = GetYChannelFromRGBImage(image);
      const cv::Mat compressed_image_y = GetYChannelFromRGBImage(compressed_image);
      const cv::Mat difference_y = imgutils::SubtractImages(compressed_image_y, image_y);
      difference_window.UpdateContent(imgutils::ConvertDifferenceImage(difference_y));
      if (difference_window.IsShown())
      {
        const double YPSNR = imgutils::PSNR(imgutils::MSE(difference_y));
        const std::string status_text = "Y-PSNR: " + comutils::FormatLevel(YPSNR);
        difference_window.ShowOverlayText(status_text);
      }
    }
    
    cv::Mat UpdateCompressedImage()
    {
      const auto quality = quality_trackbar.GetValue();
      const auto uncompressed_size = image.total() * image.elemSize();
      unsigned int compressed_size;
      cv::Mat compressed_image = CompressImage(quality, compressed_size);
      const cv::Mat combined_image = imgutils::CombineImages({image, compressed_image}, imgutils::CombinationMode::Horizontal);
      image_window.UpdateContent(combined_image);
      if (image_window.IsShown())
      {
        const std::string status_text = comutils::FormatByte(uncompressed_size) + " vs. " + comutils::FormatByte(compressed_size);
        image_window.ShowOverlayText(status_text);
      }
      return compressed_image;
    }

    static void UpdateImages(JPEG_data &data)
    {
      const cv::Mat compressed_image = data.UpdateCompressedImage();
      data.UpdateDifferenceImage(compressed_image);
    }

    static constexpr auto image_window_name = "Uncompressed vs. JPEG compressed";
    static constexpr auto quality_trackbar_name = "Quality";
    static constexpr auto difference_window_name = "Difference";
  public:
    JPEG_data(const cv::Mat &image)
     : image_window(image_window_name),
       quality_trackbar(quality_trackbar_name, image_window, 100, 0, 50, UpdateImages, *this), //50% quality by default
       difference_window(difference_window_name),
       all_windows({&image_window, &difference_window}, imgutils::WindowAlignment::Horizontal), //TODO: Align vertically, but right-aligned instead of left-aligned
       image(image)
    {
      image_window.SetAlwaysShowEnhanced(); //This window needs to be enhanced to show overlays
      difference_window.SetAlwaysShowEnhanced(); //This window needs to be enhanced to show overlays
      UpdateImages(*this); //Update with default values
    }
    
    void ShowImages()
    {
      all_windows.ShowInteractive([this]()
                                        {
                                          UpdateImages(*this); //Update again so that the status bar entries become visible
                                        });
    }
};

static void ShowImages(const cv::Mat &image)
{
  JPEG_data data(image);
  data.ShowImages();
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
