//Illustration of Haar features for object detection
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <limits>
#include <atomic>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "format.hpp"
#include "colors.hpp"
#include "window.hpp"
#include "multiwin.hpp"

class haar_data
{
  public:
    static constexpr unsigned int block_width = 100;
    static constexpr unsigned int block_height = 50;
    static_assert(block_width % 2 == 0, "Block width must be even");
    static_assert(block_height % 2 == 0, "Block height must be even");
    
    static constexpr unsigned int border_size = 1;
    static_assert(border_size < (block_width + 1) / 2, "Border size must be smaller than half the block width");
    static_assert(border_size < (block_height + 1) / 2, "Border size must be smaller than half the block height");

    static constexpr auto detection_threshold = 80000; //Feature-dependent threshold
    static_assert(detection_threshold >= 1, "The detection threshold must be positive and non-zero");
  protected:
    imgutils::Window image_window;

    using ButtonType = imgutils::Button<haar_data&>;
    ButtonType clear_button;
    ButtonType perform_button;
    ButtonType stop_button;
    ButtonType map_button;
    
    using MouseEventType = imgutils::MouseEvent<haar_data&>;
    MouseEventType image_mouse_event;
    
    imgutils::Window map_window;
    MouseEventType map_mouse_event;
    
    imgutils::MultiWindow all_windows;
 
    const cv::Mat original_image;
    const cv::Mat feature_image;
    
    cv::Mat annotated_image;
    cv::Rect current_block;
    std::atomic_bool running;

    double GetFeatureValue(const cv::Mat &block)
    {
      assert(block.size() == feature_image.size());
      assert(feature_image.type() == CV_8UC1);
      assert(feature_image.type() == block.type());
      
      cv::Mat block_signed;
      block.convertTo(block_signed, CV_16SC1);
      cv::Mat feature_signed;
      feature_image.convertTo(feature_signed, CV_16SC1, 1.0 / 127, -1); //Convert [0, 255] range to [0 / 127 - 1, 255 / 127 - 1] = [-1, 1]
      
      const cv::Mat weighted_pixels = block_signed.mul(feature_signed); //Multiply black pixels by (-1) and white pixels by 1
      const auto difference = sum(weighted_pixels); //Value is difference between weighted pixels
      const auto difference_1d = difference[0];
      return difference_1d;
    }
    
    static cv::Rect ExtendRect(const cv::Rect &rect, const unsigned int border)
    {
      return cv::Rect(rect.x - border, rect.y - border, rect.width + 2 * border, rect.height + 2 * border);
    }

    void HighlightBlock(cv::Mat image, const cv::Scalar &color)
    {
      const cv::Rect block_with_border = ExtendRect(current_block, border_size);
      cv::rectangle(image, block_with_border, color, border_size);
    }

    void OverlayBlock(cv::Mat &image)
    {
      constexpr auto overlay_alpha = 0.33;
      assert(feature_image.type() == CV_8UC1);
      cv::Mat image_region = image(current_block);
      cv::Mat converted_overlay;
      cv::cvtColor(feature_image, converted_overlay, cv::COLOR_GRAY2BGRA);
      image_region = image_region * (1 - overlay_alpha) + converted_overlay * overlay_alpha;
    }

    cv::Mat GetAnnotatedImage(const bool persist_changes)
    {
      if (persist_changes)
        HighlightBlock(annotated_image, imgutils::Green); //Draw detected faces in green and persist changes
      cv::Mat temporary_annotated_image = annotated_image.clone();
      if (!persist_changes)
        HighlightBlock(temporary_annotated_image, imgutils::Red); //Draw non-faces in red temporarily
      OverlayBlock(temporary_annotated_image);
      return temporary_annotated_image;
    }

    double UpdateImage(const bool update_GUI = true)
    {
      const cv::Mat block_pixels = original_image(current_block);
      const double feature_value = GetFeatureValue(block_pixels);
      if (update_GUI)
      {
        const auto permanent_annotation = feature_value > detection_threshold;
        const cv::Mat annotated_image = GetAnnotatedImage(permanent_annotation);
        image_window.UpdateContent(annotated_image);
        if (image_window.IsShown())
        {
          const std::string status_text = "Pixel difference (feature value): " + comutils::FormatValue(feature_value, 0);
          image_window.ShowOverlayText(status_text, true);
        }
      }
      return feature_value;
    }

    double SetCurrentPosition(const cv::Point &top_left, const bool update_GUI = true)
    {
      current_block = cv::Rect(top_left, cv::Size(block_width, block_height));
      return UpdateImage(update_GUI);
    }
    
    void ResetAnnotatedImage()
    {
      cv::cvtColor(original_image, annotated_image, cv::COLOR_GRAY2BGRA);
    }

    void ResetImage()
    {
      SetCurrentPosition(cv::Point(border_size, border_size)); //Set current position to (0, 0) plus border
    }
    
    static void ClearDetections(haar_data &data)
    {
      data.ResetAnnotatedImage();
      data.ResetImage(); //Force redraw
    }
    
    cv::Mat_<double> PerformSearch(const bool update_GUI = true)
    {
      constexpr auto search_step_delay = 1; //Animation delay in ms
      cv::Mat_<double> score_map(original_image.size(), std::numeric_limits<double>::infinity());
      for (int y = border_size; y <= original_image.cols - static_cast<int>(block_height + border_size); y++)
      {
        for (int x = border_size; x <= original_image.rows - static_cast<int>(block_width + border_size); x++)
        {
          if (update_GUI && !running) //Skip the rest when the user aborts
            return score_map;
          const cv::Point current_position(x, y);
          const double current_score = SetCurrentPosition(current_position, update_GUI);
          score_map(current_position - cv::Point(border_size, border_size)) = current_score;
          if (update_GUI)
            image_window.Wait(search_step_delay);
        }
      }
      return score_map;
    }
    
    static void PerformDetections(haar_data &data)
    {
      if (!data.running)
      {
        data.running = true;
        data.PerformSearch();
        data.running = false;
      }
    }
    
    static void StopDetections(haar_data &data)
    {
      data.running = false;
    }
    
    cv::Mat MakeColorMap(const cv::Mat_<double> &diff_map)
    {
      cv::Mat color_map(diff_map.size(), CV_8UC3);
      std::transform(diff_map.begin(), diff_map.end(), color_map.begin<cv::Vec3b>(),
                     [](const double value)
                       {
                         if (std::isinf(value))
                           return 0.5 * imgutils::White; //The border is gray
                         else if (value < detection_threshold) //Values below the threshold are more red the further away they are from the threshold
                           return ((detection_threshold - value) / (2 * detection_threshold)) * imgutils::Red; //0 is half-way, -threshold and below are full red
                         else if (value >= detection_threshold) //Values above the threshold are always 25% green and more green the further away they are from the threshold
                           return 0.25 * imgutils::Green + 0.75 * ((value - detection_threshold) / detection_threshold) * imgutils::Green;
                         else
                           return imgutils::Black;
                       });
      return color_map;
    }
    
    static void ShowMapOfDifferences(haar_data &data)
    {
      if (data.running) //Abort when the search is already running
        return;
      auto diff_map = data.PerformSearch(false);
      const auto color_map = data.MakeColorMap(diff_map);
      data.map_window.UpdateContent(color_map);
      data.map_window.Show();
    }
    
    static void SelectPointInImage(const int event, const int x, const int y, haar_data &data)
    {
      if (!data.running && event == cv::EVENT_LBUTTONUP) //Only react when the left mouse button is being pressed while no motion estimation is running
      {
        const cv::Point mouse_point(x + border_size, y + border_size); //Assume mouse position is in the top-left of the search block (on the outside border which is border_size pixels in width)
        if (mouse_point.x <= data.original_image.cols - static_cast<int>(block_width + border_size) &&
            mouse_point.y <= data.original_image.rows - static_cast<int>(block_height + border_size)) //If the mouse is within the image area (minus the positions on the bottom which would lead to the block exceeding the borders)...
          data.SetCurrentPosition(mouse_point); //... set the current position according to the mouse position
      }
    }
  
    static constexpr auto image_window_name = "Image with objects to detect";
    static constexpr auto clear_button_name = "Clear detections";
    static constexpr auto perform_button_name = "Search whole image";
    static constexpr auto stop_button_name = "Stop search";
    static constexpr auto map_button_name = "Show map of differences";
    
    static constexpr auto map_window_name = "Difference map";
  public:
    haar_data(const cv::Mat &image, const cv::Mat &feature_image)
     : image_window(image_window_name),
       clear_button(clear_button_name, image_window, ClearDetections, *this),
       perform_button(perform_button_name, image_window, PerformDetections, *this),
       stop_button(stop_button_name, image_window, StopDetections, *this),
       map_button(map_button_name, image_window, ShowMapOfDifferences, *this),
       image_mouse_event(image_window, SelectPointInImage, *this),
       map_window(map_window_name),
       map_mouse_event(map_window, SelectPointInImage, *this), //Perform the same action as in the image window
       all_windows({&image_window, &map_window}, imgutils::WindowAlignment::Horizontal, {&map_window}), //Hide map window by default
       original_image(image), feature_image(feature_image),
       current_block(cv::Rect(0, 0, block_width, block_height)), running(false)
    {
      map_window.SetPositionLikeEnhanced(); //Position this window aligned with the original (enhanced) one
      ClearDetections(*this); //Update with default values
    }
    
    void ShowImages()
    {
      all_windows.ShowInteractive();
    }
    
    static cv::Mat GetHorizontalFeatureImage()
    {
      cv::Mat feature_image(block_height, block_width, CV_8UC1, cv::Scalar(0));
      auto lower_half = feature_image(cv::Rect(0, block_height / 2, block_width, block_height / 2));
      lower_half = cv::Scalar(255);
      return feature_image;
    }
};

static void ShowImages(const cv::Mat &image, const cv::Mat &feature_image)
{
  haar_data data(image, feature_image);
  data.ShowImages();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates object detection with Haar features." << std::endl;
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
  const cv::Mat feature_image = haar_data::GetHorizontalFeatureImage();
  if (image.rows < feature_image.rows + static_cast<int>(2 * haar_data::border_size) && image.cols < feature_image.cols + static_cast<int>(2 * haar_data::border_size))
  {
    std::cerr << "The input image must be larger than " << (feature_image.rows + 2 * haar_data::border_size) << "x" << (feature_image.cols + 2 * haar_data::border_size) << " pixels" << std::endl;
    return 3;
  }
  ShowImages(image, feature_image);
  return 0;
}
