//Illustration of motion estimation and motion compensation
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <limits>
#include <algorithm>
#include <atomic>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "imgmath.hpp"
#include "format.hpp"
#include "colors.hpp"
#include "window.hpp"
#include "multiwin.hpp"

class ME_data
{
  public:
    static constexpr unsigned int search_radius = 16;
    static constexpr unsigned int block_size = 8;
    static_assert(search_radius >= block_size, "Search radius must be larger than block size");

    static constexpr unsigned int border_size = 1;
    static_assert(border_size < (block_size + 1) / 2, "Border size must be smaller than half the block size");
  protected:
    imgutils::Window ME_window;
    
    using ButtonType = imgutils::Button<ME_data&>;
    ButtonType perform_button;
    ButtonType stop_button;
    ButtonType map_button;
    
    using MouseEventType = imgutils::MouseEvent<ME_data&>;
    MouseEventType ME_mouse_event;
    
    imgutils::Window MC_window;
        
    imgutils::Window map_window;
    MouseEventType map_mouse_event;
    
    imgutils::MultiWindow MC_map_window;
    imgutils::MultiWindow all_windows;
  
    const cv::Mat reference_image;
    const cv::Mat image;
    const cv::Rect search_area;
    const cv::Rect reference_block;
    
    cv::Point relative_search_position;
    std::atomic_bool running;
    
    static cv::Rect ExtendRect(const cv::Rect &rect, const unsigned int border)
    {
      return cv::Rect(rect.x - border, rect.y - border, rect.width + 2 * border, rect.height + 2 * border);
    }

    static void HighlightBlock(cv::Mat &image, const cv::Rect &block, const cv::Scalar &color)
    {
      const cv::Rect block_with_border = ExtendRect(block, border_size);
      cv::rectangle(image, block_with_border, color, border_size);
    }

    cv::Rect GetSearchedBlock() const
    {
      const cv::Rect searched_block(reference_block.tl() + relative_search_position, reference_block.size());
      return searched_block;
    }

    cv::Mat GetAnnotatedReferenceImage(const cv::Rect &searched_block) const
    {
      cv::Mat annotated_reference_image;
      cv::cvtColor(reference_image, annotated_reference_image, cv::COLOR_GRAY2BGR);
      HighlightBlock(annotated_reference_image, search_area, imgutils::Green);
      HighlightBlock(annotated_reference_image, reference_block, imgutils::Blue);
      HighlightBlock(annotated_reference_image, searched_block, imgutils::Red); //Searched position
      return annotated_reference_image;
    }

    cv::Mat GetAnnotatedImage() const
    {
      cv::Mat annotated_image;
      cv::cvtColor(image, annotated_image, cv::COLOR_GRAY2BGR);
      HighlightBlock(annotated_image, reference_block, imgutils::Blue);
      return annotated_image;
    }

    cv::Rect UpdateMotionEstimationImage(const bool update_GUI = true)
    {
      const cv::Rect searched_block = GetSearchedBlock();
      if (update_GUI)
      {
        const cv::Mat annotated_reference_image = GetAnnotatedReferenceImage(searched_block);
        const cv::Mat annotated_image = GetAnnotatedImage();
        const cv::Mat combined_image = imgutils::CombineImages({annotated_reference_image, annotated_image}, imgutils::CombinationMode::Horizontal);
        ME_window.UpdateContent(combined_image);
        if (ME_window.IsShown())
        {
          const std::string status_text = "Motion vector: (" + std::to_string(relative_search_position.x) + ", " + std::to_string(relative_search_position.y) + ")";
          ME_window.ShowOverlayText(status_text);
        }
      }
      return searched_block;
    }

    static std::string GetDifferenceMetrics(const cv::Mat &difference, double &representative_metric_value)
    {
      const double YSAD = imgutils::SAD(difference);
      const double YSSD = imgutils::SSD(difference);
      representative_metric_value = YSSD;
      const double YMSE = YSSD / (block_size * block_size);
      const double YPSNR = imgutils::PSNR(YMSE);
      return "SAD: " + comutils::FormatValue(YSAD) + ", SSD: " + comutils::FormatValue(YSSD) + ", MSE: " + comutils::FormatValue(YMSE) + ", Y-PSNR: " + comutils::FormatLevel(YPSNR);
    }

    double UpdateMotionCompensationImage(const cv::Rect &searched_block, const bool update_GUI = true)
    {
      const cv::Mat searched_block_pixels = reference_image(searched_block);
      const cv::Mat block_pixels = image(reference_block);
      const cv::Mat compensated_block_pixels_16 = imgutils::SubtractImages(searched_block_pixels, block_pixels);
      double difference_value;
      const std::string status_text = GetDifferenceMetrics(compensated_block_pixels_16, difference_value);
      if (update_GUI)
      {
        const cv::Mat difference_image = imgutils::ConvertDifferenceImage(compensated_block_pixels_16);
        const cv::Mat combined_image = imgutils::CombineImages({searched_block_pixels, block_pixels, difference_image}, imgutils::CombinationMode::Horizontal, 1);
        MC_window.UpdateContent(combined_image);
        MC_window.ZoomFully();
        if (MC_window.IsShown())
          MC_window.ShowOverlayText(status_text, true);
      }
      return difference_value;
    }

    double UpdateImages(const bool update_GUI = true)
    {
      const cv::Rect searched_block = UpdateMotionEstimationImage(update_GUI);
      return UpdateMotionCompensationImage(searched_block, update_GUI);
    }
    
    static cv::Rect LimitRect(const cv::Rect &rect, const unsigned int distance)
    {
      return cv::Rect(rect.tl(), rect.size() - cv::Size(distance - 1, distance -1)); //Shrink rectangle, but leave one pixel on the right and the bottom for the collision check to work (it considers the bottom-right pixels to be outside the rectangle)
    }

    double SetMotionVector(const cv::Point &MV, const bool update_GUI = true)
    {
      relative_search_position = MV;
      return UpdateImages(update_GUI);
    }

    static cv::Rect ExtendRect(const cv::Point &center, const unsigned int border)
    {
      return cv::Rect(center.x - border, center.y - border, 2 * border, 2 * border);
    }

    static constexpr int search_limit = static_cast<int>(search_radius) - block_size / 2;
    static const cv::Point search_index_offset; //Defined outside the class as cv::Point is not constexpr

    static cv::Point MVToMatrixPosition(const cv::Point &p)
    {
      const cv::Point pos = p + search_index_offset;
      return pos;
    }

    static cv::Point MatrixPositionToMV(const cv::Point &p)
    {
      const cv::Point offset(search_limit, search_limit);
      const cv::Point pos = p - search_index_offset;
      return pos;
    }

    static constexpr auto search_pixels = 2 * search_limit + 1;

    cv::Mat_<double> PerformMotionEstimation(const bool update_GUI = true)
    {
      constexpr auto ME_step_delay = 10; //Animation delay in ms
      cv::Mat_<double> cost_map(search_pixels, search_pixels, std::numeric_limits<double>::infinity());
      for (int y = -search_limit; y <= search_limit; y++)
      {
        for (int x = -search_limit; x <= search_limit; x++)
        {
          if (update_GUI && !running) //Skip the rest when the user aborts
            return cost_map;
          const cv::Point current_MV(x, y);
          const double current_cost = SetMotionVector(current_MV, update_GUI);
          cost_map(MVToMatrixPosition(current_MV)) = current_cost;
          if (update_GUI)
            ME_window.Wait(ME_step_delay);
        }
      }
      return cost_map;
    }

    void SetBestMV(const cv::Mat_<double> &cost_map)
    {
      assert(!cost_map.empty());
      const auto min_it = std::min_element(cost_map.begin(), cost_map.end());
      const auto min_position = min_it.pos();
      const auto min_MV = MatrixPositionToMV(min_position);
      SetMotionVector(min_MV);
    }
    
    static void PerformME(ME_data &data)
    {
      if (!data.running)
      {
        data.running = true;
        const auto cost_map = data.PerformMotionEstimation();
        if (data.running) //If the user did not abort...
          data.SetBestMV(cost_map); //... set the best MV from the search
        data.running = false;
      }
    }
    
    static void StopME(ME_data &data)
    {
      data.running = false;
    }
    
    static cv::Mat MakeGrayscaleMap(const cv::Mat_<double> &cost_map)
    {
      cv::Mat grayscale_map(cost_map.size(), CV_8UC1);
      normalize(cost_map, grayscale_map, 0, 255, cv::NORM_MINMAX, CV_8UC1); //Shift and scale the map values so that the minimum becomes 0 and the maximum becomes 255
      return grayscale_map;
    }
    
    static void ShowMapOfCosts(ME_data &data)
    {
      constexpr auto scale_factor = 10; //10x zoom
      if (data.running) //Abort when the ME is already running
        return;
      auto cost_map = data.PerformMotionEstimation(false);
      const auto grayscale_map = MakeGrayscaleMap(cost_map);
      data.map_window.SetSize(grayscale_map.size() * scale_factor);
      data.map_window.UpdateContent(grayscale_map);
      data.map_window.Show();
    }
    
    static void MEMouseEvent(const int event, const int x, const int y, ME_data &data)
    {
      if (!data.running && event == cv::EVENT_LBUTTONUP) //Only react when the left mouse button is being pressed while no motion estimation is running
      {
        const cv::Point mouse_point(x + border_size, y + border_size); //Assume mouse position is in the top-left of the search block (on the outside border which is border_size pixels in width)
        if (LimitRect(data.search_area, block_size).contains(mouse_point)) //If the mouse is within the search area (minus the positions on the bottom which would lead to the search block exceeding the borders)...
          data.SetMotionVector(mouse_point - data.reference_block.tl()); //... set the search position according to the mouse position
      }
    }
    
    static void MapMouseEvent(const int event, const int x, const int y, ME_data &data)
    {
      if (!data.running && event == cv::EVENT_LBUTTONUP) //Only react when the left mouse button is being pressed while no motion estimation is running
      {
        const cv::Point mouse_point(x, y);
        const cv::Point MV = MatrixPositionToMV(mouse_point);
        data.SetMotionVector(MV);
      }
    }
    
    static constexpr auto ME_window_name = "Motion estimation";
    static constexpr auto perform_button_name = "Perform ME";
    static constexpr auto stop_button_name = "Stop ME";
    static constexpr auto map_button_name = "Show map of costs";
    
    static constexpr auto MC_window_name = "Found block vs. original block vs. motion compensation";
    static constexpr auto map_window_name = "Cost map (SSD values)";
  public:  
    ME_data(const cv::Mat &reference_image, const cv::Mat &image, const cv::Point &block_center)
     : ME_window(ME_window_name),
       //TODO: Add option to only show the search range in both frames
       perform_button(perform_button_name, ME_window, PerformME, *this),
       stop_button(stop_button_name, ME_window, StopME, *this),
       map_button(map_button_name, ME_window, ShowMapOfCosts, *this),
       ME_mouse_event(ME_window, MEMouseEvent, *this),
       MC_window(MC_window_name),
       map_window(map_window_name),
       map_mouse_event(map_window, MapMouseEvent, *this),
       MC_map_window({&MC_window, &map_window}, imgutils::WindowAlignment::Vertical, {&map_window}), //Hide map window by default
       all_windows({&ME_window, &MC_map_window}, imgutils::WindowAlignment::Horizontal),
       reference_image(reference_image), image(image),
       search_area(ExtendRect(block_center, search_radius)),
       reference_block(ExtendRect(block_center, block_size / 2)),
       relative_search_position(cv::Point()), //Set MV to (0, 0)
       running(false)
    {
      assert(reference_image.size() == image.size());
      MC_window.SetAlwaysShowEnhanced(); //The MC window needs to be enhanced to show overlays
      UpdateImages(); //Update with default values
    }

    void ShowImages()
    {
      all_windows.ShowInteractive([this]()
                                        {
                                          UpdateImages(); //Update again so that the status bar entries become visible
                                        });
    }
};

const cv::Point ME_data::search_index_offset = cv::Point(search_limit, search_limit);

static void ShowImages(const cv::Mat &reference_image, const cv::Mat &image, const cv::Point &block_center)
{
  ME_data data(reference_image, image, block_center);
  data.ShowImages();
}

static int CheckParameters(const cv::Mat &reference_image, const cv::Mat &image, const cv::Point &block_origin)
{
  if (reference_image.size() != image.size())
  {
    std::cerr << "Both images must have the same size" << std::endl;
    return 10;
  }
  
  const auto max_search_radius = 2 * ME_data::search_radius;
  if (static_cast<unsigned int>(reference_image.rows) < max_search_radius)
  {
    std::cerr << "The images must be larger than " << max_search_radius << " pixels in each dimension" << std::endl;
    return 11;
  }
  
  const auto max_x_origin = reference_image.cols - ME_data::search_radius - 1;
  if (static_cast<unsigned int>(block_origin.x) < ME_data::search_radius || static_cast<unsigned int>(block_origin.x) > max_x_origin)
  {
    std::cerr << "Block center X coordinate must be between " << ME_data::search_radius << " and " << max_x_origin << std::endl;
    return 12;
  }
  const auto max_y_origin = reference_image.rows - ME_data::search_radius - 1;
  if (static_cast<unsigned int>(block_origin.y) < ME_data::search_radius || static_cast<unsigned int>(block_origin.y) > max_y_origin)
  {
    std::cerr << "Block center Y coordinate must be between " << ME_data::search_radius << " and " << max_y_origin << std::endl;
    return 13;
  }
  return 0;
}

int main(const int argc, const char * const argv[])
{
  if (argc != 5)
  {
    std::cout << "Illustrates motion estimation and motion compensation." << std::endl;
    std::cout << "Usage: " << argv[0] << " <reference image> <input image> <block center X coordinate> <block center Y coordinate>" << std::endl;
    return 1;
  }
  const auto reference_image_filename = argv[1];
  const cv::Mat reference_image = cv::imread(reference_image_filename, cv::IMREAD_GRAYSCALE);
  if (reference_image.empty())
  {
    std::cerr << "Could not read reference image '" << reference_image_filename << "'" << std::endl;
    return 2;
  }
  const auto image_filename = argv[2];
  const cv::Mat image = cv::imread(image_filename, cv::IMREAD_GRAYSCALE);
  if (image.empty())
  {
    std::cerr << "Could not read input image '" << reference_image_filename << "'" << std::endl;
    return 3;
  }
  cv::Point block_origin;
  const auto x_coordinate = argv[3];
  block_origin.x = std::stoi(x_coordinate);
  const auto y_coordinate = argv[4];
  block_origin.y = std::stoi(y_coordinate);
  int ret;
  if ((ret = CheckParameters(reference_image, image, block_origin)) != 0)
    return ret;
  ShowImages(reference_image, image, block_origin);
  return 0;
}
