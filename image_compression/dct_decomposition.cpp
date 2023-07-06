//Illustration of the decomposition of a block into 2-D DCT basis functions and their recomposition
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <cassert>
#include <vector>
#include <atomic>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "math.hpp"
#include "imgmath.hpp"
#include "colors.hpp"
#include "combine.hpp"
#include "format.hpp"
#include "window.hpp"
#include "multiwin.hpp"

class DCT_data
{
  public:
    static constexpr unsigned int log_max_block_size = 6; //(1 << 6) = 64
    static constexpr unsigned int max_block_size = 1 << log_max_block_size; //Maximum DCT size (2^log_max_block_size)

    static constexpr unsigned int log_default_block_size = 3; //(1 << 3) = 8
    static_assert(log_default_block_size <= log_max_block_size, "The default block size cannot be larger than the maximum block size");  
    
    static constexpr auto displayed_window_dimension = 400;
    static const cv::Size displayed_window_size; //Defined below
  protected:
    imgutils::Window decomposition_window;
    
    using TrackBarType = imgutils::TrackBar<DCT_data&>;
    TrackBarType block_size_trackbar;
    
    using ButtonType = imgutils::Button<DCT_data&>;
    ButtonType start_button;
    ButtonType stop_button;
    
    using MouseEventType = imgutils::MouseEvent<DCT_data&>;
    MouseEventType decomposition_mouse_event;
    
    imgutils::Window detail_window;
    
    imgutils::Window sum_window;
    
    imgutils::MultiWindow all_windows;
    
    const cv::Mat image;
    std::atomic_bool running;
    
    unsigned int GetBlockSize()
    {
      const auto log_block_size = block_size_trackbar.GetValue();
       return 1U << log_block_size; //2^(log_block_size)
    }
    
    static cv::Mat Decompose(const cv::Mat &image, cv::Mat_<double> &raw_coefficients)
    {
      assert(image.cols == image.rows);
      const unsigned int block_size = static_cast<unsigned int>(image.rows);
      const cv::Mat shifted_image = imgutils::ImageLevelShift(image);
      cv::dct(shifted_image, raw_coefficients);
      raw_coefficients.forEach([block_size](double &value, const int position[])
                                           {
                                             value *= comutils::Get2DDCTCoefficientScalingFactor(block_size, position[0], position[1]);
                                           });
      cv::Mat decomposed_image = imgutils::ReverseImageLevelShift(raw_coefficients);
      return decomposed_image;
    }

    static cv::Mat HighlightCoefficient(const cv::Mat &decomposed_image, const unsigned int highlighted_x_index, const unsigned int highlighted_y_index)
    {
      assert(decomposed_image.type() == CV_8UC1);
      cv::Mat image_highlighted;
      cv::cvtColor(decomposed_image, image_highlighted, cv::COLOR_GRAY2BGR); //Convert image to color so that coefficients can be highlighted
      auto &pixel = image_highlighted.at<cv::Vec3b>(highlighted_y_index, highlighted_x_index);
      pixel[0] = 0; //Set B zero
      pixel[1] = 0; //Set G zero => R remains, highlighting in red with the previous grayscale value
      return image_highlighted;
    }

    double UpdateImageAndDCT(const unsigned int highlighted_x_index, const unsigned int highlighted_y_index)
    {
      const cv::Mat image_part = GetCenterBlock();
      cv::Mat_<double> coefficients;
      const cv::Mat decomposed_image = Decompose(image_part, coefficients);
      const cv::Mat decomposed_image_highlighted = HighlightCoefficient(decomposed_image, highlighted_x_index, highlighted_y_index);
      const cv::Mat combined_image = imgutils::CombineImages({image_part, decomposed_image_highlighted}, imgutils::CombinationMode::Horizontal, 1);
      decomposition_window.UpdateContent(combined_image);
      decomposition_window.SetSize(displayed_window_size);
      return coefficients(highlighted_y_index, highlighted_x_index);
    }

    cv::Mat UpdateWeightedBasisFunctionImage(const unsigned int x_index, const unsigned int y_index, const double value)
    {
      const auto shifted_value = imgutils::ReverseLevelShift(value);
      const auto sanitized_shifted_value = std::max(0.0, std::min(shifted_value, 255.0)); //Make sure the level is within the 8-bit range [0;255] in case of rounding and/or floating-point errors
      const auto block_size = GetBlockSize();
      const cv::Mat basis_function = imgutils::Get2DDCTBasisFunctionImage(block_size, y_index, x_index);
      const cv::Mat raw_weighted_basis_function = imgutils::GetRaw2DDCTBasisFunctionImage(block_size, y_index, x_index, sanitized_shifted_value); //For calculating sums (not for illustration)
      const cv::Mat weighted_basis_function = imgutils::Get2DDCTBasisFunctionImage(block_size, y_index, x_index, sanitized_shifted_value);
      const cv::Mat combined_image = imgutils::CombineImages({basis_function, weighted_basis_function}, imgutils::CombinationMode::Horizontal, 1);
      detail_window.UpdateContent(combined_image);
      detail_window.SetSize(displayed_window_size);
      if (detail_window.IsShown())
      {
        const std::string status_text = "Coefficient (" + std::to_string(x_index) + ", " + std::to_string(y_index) + "): " + std::to_string(value);
        detail_window.ShowOverlayText(status_text, true);
      }
      return raw_weighted_basis_function;
    }
    
    cv::Mat GetCenterBlock()
    {
      const int block_size = GetBlockSize();
      const cv::Point center_point(image.rows / 2, image.cols / 2);
      const cv::Rect center_rect(center_point, cv::Size(block_size, block_size));
      const cv::Mat center_block = image(center_rect);
      return center_block;
    }

    cv::Mat SetFocusedCoefficient(const unsigned int x_index, const unsigned int y_index)
    {
      const auto value = UpdateImageAndDCT(x_index, y_index);
      const auto raw_weighted_basis_function_image = UpdateWeightedBasisFunctionImage(x_index, y_index, value);
      return raw_weighted_basis_function_image;
    }
    
    void ResetWindows()
    {
      const int block_size = GetBlockSize();
      cv::Mat empty_sum(block_size, block_size, CV_8UC1, cv::Scalar(imgutils::ReverseLevelShift(0))); //Create level-shifted zero image
      sum_window.UpdateContent(empty_sum); //Show all-grey image
      sum_window.SetSize(displayed_window_size);
      if (sum_window.IsShown())
        sum_window.ShowOverlayText("Please start adding via the corresponding button.", true);
      SetFocusedCoefficient(0, 0); //Set focus to DC coefficient
    }
    
    static void UpdateImages(DCT_data &data)
    {
      data.running = false; //Make sure the animation is stopped
      data.ResetWindows();
    }
    
    using coefficient_index = std::pair<size_t, size_t>;

    std::vector<coefficient_index> ZigZagScanIndices()
    {
      const auto block_size = GetBlockSize();
      assert(block_size >= 1);
      size_t x = 0, y = 0;
      bool up = true;
      std::vector<coefficient_index> indices;
      indices.reserve(block_size * block_size);
      indices.push_back(coefficient_index(0, 0));
      while (!(x == block_size - 1 && y == block_size - 1))
      {
        while(up)
        {
          if (y == 0 || x == block_size - 1)
          {
            if (y == 0)
              x++;
            else if (x == block_size - 1)
              y++;
            indices.push_back(coefficient_index(x, y));
            up = false;
          }
          else
          {
            x++;
            y--;
            indices.push_back(coefficient_index(x, y));
          }
        }
        while (!up)
        {
          if (x == 0 || y == block_size - 1)
          {
            if (y == block_size - 1)
              x++;
            else if (x == 0)
              y++;
            indices.push_back(coefficient_index(x, y));
            up = true;
          }
          else
          {
            x--;
            y++;
            indices.push_back(coefficient_index(x, y));
          }
        }
      }
      return indices;
    }
    
    void AddWeightedBasisFunctions()
    {
      constexpr auto step_delay = 5000; //Animation delay in ms
      const int block_size = GetBlockSize();
      cv::Mat raw_sum(block_size, block_size, CV_64FC1, cv::Scalar(0.0)); //Initialize sum with zeros
      unsigned int coefficient = 0;
      const auto indices = ZigZagScanIndices();
      for (const auto &index : indices)
      {
        if (!running) //Skip the rest when the user aborts
          return;
        const auto x = index.first;
        const auto y = index.second;
        const auto raw_weighted_basis_function_image = SetFocusedCoefficient(x, y);
        raw_sum += raw_weighted_basis_function_image; //Add image to sum
        const cv::Mat sum = imgutils::ReverseImageLevelShift(raw_sum);
        sum_window.UpdateContent(sum);
        sum_window.SetSize(displayed_window_size);
        coefficient++;
        const std::string status_text = std::to_string(coefficient) + " of " + std::to_string(block_size * block_size) + " coefficients (" + comutils::FormatValue((100.0 * coefficient) / (block_size * block_size)) + "%)";
        sum_window.ShowOverlayText(status_text, true);
        sum_window.Wait(std::max(1.0, (double)step_delay / coefficient)); //Move faster for higher-frequency coefficients (minimum 1 since a keypress would be required otherwise)
      }
    }
    
    static void StartSumming(DCT_data &data)
    {
      if (!data.running)
      {
        data.running = true;
        data.AddWeightedBasisFunctions();
        data.running = false;
      }
    }
    
    static void StopSumming(DCT_data &data)
    {
      data.running = false;
    }
    
    static void DecompositionMouseEvent(const int event, const int x, const int y, DCT_data &data)
    {
      if (event == cv::EVENT_LBUTTONUP) //Only react when the left mouse button is being pressed
      {
        const auto block_size = data.GetBlockSize();
        const int relative_x = x - block_size - 1; //Calculate relative coefficient position in image (original image and additional 1 pixel border)
        const int relative_y = y;
        if (relative_x >= 0 && static_cast<unsigned int>(relative_x) < block_size && relative_y >= 0 && static_cast<unsigned int>(relative_y) < block_size)
          data.SetFocusedCoefficient(relative_x, relative_y);
      }
    }
    
    static constexpr auto decomposition_window_name = "DCT decomposition";
    static constexpr auto block_size_trackbar_name = "log2(transform size)";
    static constexpr auto detail_window_name = "Associated basis function";
    static constexpr auto sum_window_name = "Sum of weighted basis functions";
    static constexpr auto start_button_name = "Add weighted basis functions";
    static constexpr auto stop_button_name = "Stop animation";
  public:
    DCT_data(const cv::Mat &image)
     : decomposition_window(decomposition_window_name),
       block_size_trackbar(block_size_trackbar_name, decomposition_window, log_max_block_size, 0, log_default_block_size, UpdateImages, *this),
       start_button(start_button_name, decomposition_window, StartSumming, *this),
       stop_button(stop_button_name, decomposition_window, StopSumming, *this),
       decomposition_mouse_event(decomposition_window, DecompositionMouseEvent, *this),
       detail_window(detail_window_name),
       sum_window(sum_window_name),
       all_windows({&decomposition_window, &detail_window, &sum_window}, imgutils::WindowAlignment::Horizontal),
       image(image),
       running(false)
    {
      detail_window.SetAlwaysShowEnhanced(); //This window needs to be enhanced to show overlays
      sum_window.SetAlwaysShowEnhanced(); //This window needs to be enhanced to show overlays
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

const cv::Size DCT_data::displayed_window_size = cv::Size(displayed_window_dimension, displayed_window_dimension / 2); //2:1 aspect ratio //TODO: The width is not correct by a few pixels due to the separator line; correct this

static void ShowImages(const cv::Mat &image)
{
  DCT_data data(image);
  data.ShowImages();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the DCT components of a block and their reassembly" << std::endl;
    std::cout << "Usage: " << argv[0] << " <input image>" << std::endl;
    return 1;
  }
  const auto image_path = argv[1];
  const cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
  if (image.empty())
  {
	  std::cerr << "Could not read input image '" << image_path << "'" << std::endl;
	  return 2;
  }
  if (static_cast<unsigned int>(std::min(image.rows, image.cols)) < DCT_data::max_block_size)
  {
	  std::cerr << "The input image must be at least " << DCT_data::max_block_size << "x" << DCT_data::max_block_size << " pixels in size" << std::endl;
    return 3;
  }
  ShowImages(image);
  return 0;
}
