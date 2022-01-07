//Illustration of the decomposition of a block into 2-D DCT basis functions and their recomposition
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <cassert>
#include <atomic>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "math.hpp"
#include "imgmath.hpp"
#include "colors.hpp"
#include "combine.hpp"
#include "format.hpp"

static constexpr unsigned int log_max_block_size = 6; //(1 << 6) = 64
static constexpr unsigned int max_block_size = 1 << log_max_block_size; //Maximum DCT size (2^log_max_block_size)

static constexpr unsigned int log_default_block_size = 3; //(1 << 3) = 8
static_assert(log_default_block_size <= log_max_block_size, "The default block size cannot be larger than the maximum block size");

struct DCT_data
{
  const cv::Mat image;
  std::atomic_bool running;
  
  const std::string window_name;
  const std::string detail_window_name;
  const std::string sum_window_name;
  
  static constexpr auto trackbar_name = "log2(transform size)";
  
  DCT_data(const cv::Mat &image, const std::string &window_name, const std::string &detail_window_name, const std::string &sum_window_name)
   : image(image), 
     running(false),
     window_name(window_name), detail_window_name(detail_window_name), sum_window_name(sum_window_name) { }
};

static cv::Mat GetCenterBlock(const cv::Mat &image, const unsigned int block_size)
{
  const cv::Point center_point(image.rows / 2, image.cols / 2);
  const cv::Rect center_rect(center_point, cv::Size(block_size, block_size));
  const cv::Mat center_block = image(center_rect);
  return center_block;
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

static cv::Mat HighlightCoefficient(const cv::Mat &image, const unsigned int highlighted_x_index, const unsigned int highlighted_y_index)
{
  assert(image.type() == CV_8UC1);
  cv::Mat image_highlighted;
  cv::cvtColor(image, image_highlighted, cv::COLOR_GRAY2BGR); //Convert image to color so that coefficients can be highlighted
  auto &pixel = image_highlighted.at<cv::Vec3b>(highlighted_y_index, highlighted_x_index);
  pixel[0] = 0; //Set B zero
  pixel[1] = 0; //Set G zero => R remains, highlighting in red with the previous grayscale value
  return image_highlighted;
}

static double ShowImageAndDCT(const cv::Mat &image, const unsigned int highlighted_x_index, const unsigned int highlighted_y_index, const std::string &window_name)
{
  cv::Mat_<double> coefficients;
  const cv::Mat decomposed_image = Decompose(image, coefficients);
  const cv::Mat decomposed_image_highlighted = HighlightCoefficient(decomposed_image, highlighted_x_index, highlighted_y_index);
  const cv::Mat combined_image = imgutils::CombineImages({image, decomposed_image_highlighted}, imgutils::CombinationMode::Horizontal, 1);
  cv::imshow(window_name, combined_image);
  return coefficients(highlighted_y_index, highlighted_x_index);
}

static cv::Mat ShowWeightedBasisFunctionImage(const unsigned int x_index, const unsigned int y_index, const double value, unsigned int block_size, const std::string &window_name)
{
  const auto shifted_value = imgutils::ReverseLevelShift(value);
  const auto sanitized_shifted_value = std::max(0.0, std::min(shifted_value, 255.0)); //Make sure the level is within the 8-bit range [0;255] in case of rounding and/or floating-point errors
  const cv::Mat basis_function = imgutils::Get2DDCTBasisFunctionImage(block_size, y_index, x_index);
  const cv::Mat raw_weighted_basis_function = imgutils::GetRaw2DDCTBasisFunctionImage(block_size, y_index, x_index, sanitized_shifted_value); //For calculating sums (not for illustration)
  const cv::Mat weighted_basis_function = imgutils::Get2DDCTBasisFunctionImage(block_size, y_index, x_index, sanitized_shifted_value);
  const cv::Mat combined_image = imgutils::CombineImages({basis_function, weighted_basis_function}, imgutils::CombinationMode::Horizontal, 1);
  const std::string status_text = "Coefficient (" + std::to_string(x_index) + ", " + std::to_string(y_index) + "): " + std::to_string(value);
  //std::displayOverlay(window_name, status_text, 1000);
  cv::displayStatusBar(window_name, status_text);
  cv::imshow(window_name, combined_image);
  return raw_weighted_basis_function;
}

static cv::Mat SetFocusedCoefficient(const unsigned int x_index, const unsigned int y_index, const DCT_data &data, const int block_size)
{
  const cv::Mat image_part = GetCenterBlock(data.image, block_size);
  const auto value = ShowImageAndDCT(image_part, x_index, y_index, data.window_name);
  const auto raw_weighted_basis_function_image = ShowWeightedBasisFunctionImage(x_index, y_index, value, block_size, data.detail_window_name);
  return raw_weighted_basis_function_image;
}

using coefficient_index = std::pair<size_t, size_t>;

static std::vector<coefficient_index> ZigZagScanIndices(unsigned int block_size)
{
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

static void AddWeightedBasisFunctions(const DCT_data &data, const int block_size)
{
  constexpr auto step_delay = 5000; //Animation delay in ms
  cv::Mat raw_sum(block_size, block_size, CV_64FC1, cv::Scalar(0.0)); //Initialize sum with zeros
  unsigned int coefficient = 0;
  const auto indices = ZigZagScanIndices(block_size);
  for (const auto &index : indices)
  {
    if (!data.running) //Skip the rest when the user aborts
      return;
    const auto x = index.first;
    const auto y = index.second;
    const auto raw_weighted_basis_function_image = SetFocusedCoefficient(x, y, data, block_size);
    raw_sum += raw_weighted_basis_function_image; //Add image to sum
    const cv::Mat sum = imgutils::ReverseImageLevelShift(raw_sum);
    cv::imshow(data.sum_window_name, sum);
    coefficient++;
    const std::string status_text = std::to_string(coefficient) + " of " + std::to_string(block_size * block_size) + " coefficients (" + comutils::FormatValue((100.0 * coefficient) / (block_size * block_size)) + "%)";
    cv::displayStatusBar(data.sum_window_name, status_text);
    cv::waitKey(std::max(1.0, (double)step_delay / coefficient)); //Move faster for higher-frequency coefficients (minimum 1 since a keypress would be required otherwise)
  }
}

static unsigned int GetBlockSize(const unsigned int log_block_size)
{
   return 1U << log_block_size; //2^(log_block_size)
}

static void ShowImages(const cv::Mat &image)
{
  constexpr auto window_name = "DCT decomposition";
  cv::namedWindow(window_name, cv::WINDOW_NORMAL);
  constexpr auto detail_window_name = "Associated basis function";
  cv::namedWindow(detail_window_name, cv::WINDOW_NORMAL);
  constexpr auto sum_window_name = "Sum of weighted basis functions";
  cv::namedWindow(sum_window_name, cv::WINDOW_NORMAL);
  static DCT_data data(image, window_name, detail_window_name, sum_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  cv::createTrackbar(data.trackbar_name, window_name, nullptr, log_max_block_size, [](const int log_block_size, void * const user_data)
                                                                                     {
                                                                                       auto &data = *(static_cast<DCT_data*>(user_data));
                                                                                       data.running = false; //Make sure the animation is stopped
                                                                                       const int block_size = GetBlockSize(log_block_size);
                                                                                       cv::Mat empty_sum(block_size, block_size, CV_8UC1, cv::Scalar(imgutils::ReverseLevelShift(0))); //Create level-shifted zero image
                                                                                       cv::imshow(data.sum_window_name, empty_sum); //Show all-grey image
                                                                                       cv::displayStatusBar(data.sum_window_name, "Please start adding via the corresponding button.");
                                                                                       SetFocusedCoefficient(0, 0, data, block_size); //Set focus to DC coefficient (implies cv::imshow)
                                                                                       constexpr auto displayed_window_size = 250;
                                                                                       cv::resizeWindow(data.window_name, 2 * displayed_window_size, displayed_window_size);
                                                                                       cv::moveWindow(data.window_name, 0, 0);
                                                                                       cv::resizeWindow(data.detail_window_name, 2 * displayed_window_size, displayed_window_size);
                                                                                       cv::moveWindow(data.detail_window_name, 2 * displayed_window_size + 3, 0); //Move basis function window right beside the DCT window (window width plus additional distance)
                                                                                       cv::resizeWindow(data.sum_window_name, displayed_window_size, displayed_window_size);
                                                                                       cv::moveWindow(data.sum_window_name, 2 * (2 * displayed_window_size + 3), 0); //Move basis function window right beside the DCT and basis function windows (two window widths plus additional distance, each)
                                                                                      }, static_cast<void*>(&data));
  cv::setMouseCallback(window_name, [](const int event, const int x, const int y, const int, void * const userdata)
                                      {
                                        auto &data = *(static_cast<const DCT_data*>(userdata));
                                        if (event == cv::EVENT_LBUTTONUP) //Only react when the left mouse button is being pressed
                                        {
                                          const auto log_block_size = cv::getTrackbarPos(data.trackbar_name, data.window_name);
                                          const auto block_size = GetBlockSize(log_block_size);
                                          const int relative_x = x - block_size - 1; //Calculate relative coefficient position in image (original image and additional 1 pixel border)
                                          const int relative_y = y;
                                          if (relative_x >= 0 && static_cast<unsigned int>(relative_x) < block_size && relative_y >= 0 && static_cast<unsigned int>(relative_y) < block_size)
                                            SetFocusedCoefficient(relative_x, relative_y, data, block_size);
                                        }
                                      }, static_cast<void*>(&data));
  constexpr auto start_button_name = "Add weighted basis functions";
  cv::createButton(start_button_name, [](const int, void * const user_data)
                                        {
                                          auto &data = *(static_cast<DCT_data*>(user_data));
                                          if (!data.running)
                                          {
                                            data.running = true;
                                            const auto log_block_size = cv::getTrackbarPos(data.trackbar_name, data.window_name);
                                            const auto block_size = GetBlockSize(log_block_size);
                                            AddWeightedBasisFunctions(data, block_size);
                                            data.running = false;
                                          }
                                        }, static_cast<void*>(&data), cv::QT_PUSH_BUTTON);
  constexpr auto stop_button_name = "Stop animation";
  cv::createButton(stop_button_name, [](const int, void * const user_data)
                                       {
                                         auto &data = *(static_cast<DCT_data*>(user_data));
                                         data.running = false;
                                       }, static_cast<void*>(&data), cv::QT_PUSH_BUTTON);
  cv::setTrackbarPos(data.trackbar_name, window_name, log_default_block_size); //Implies cv::imshow with DCT size of 8
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
  if (static_cast<unsigned int>(std::min(image.rows, image.cols)) < max_block_size)
  {
	  std::cerr << "The input image must be at least " << max_block_size << "x" << max_block_size << " pixels in size" << std::endl;
    return 3;
  }
  ShowImages(image);
  cv::waitKey(0);
  return 0;
}
