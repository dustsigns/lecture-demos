//Illustration of Haar features for object detection
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <limits>
#include <atomic>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "format.hpp"
#include "colors.hpp"

static constexpr unsigned int block_width = 100;
static constexpr unsigned int block_height = 50;
static_assert(block_width % 2 == 0, "Block width must be even");
static_assert(block_height % 2 == 0, "Block height must be even");

struct haar_data
{
  const cv::Mat feature_image;
  const cv::Mat original_image;
  cv::Mat annotated_image;
  cv::Rect current_block;
  std::atomic_bool running;
  
  const std::string window_name;
  
  void ResetAnnotatedImage()
  {
    cv::cvtColor(original_image, annotated_image, cv::COLOR_GRAY2BGRA);
  }
  
  haar_data(const cv::Mat &feature_image, const cv::Mat &original_image, const std::string &window_name)
   : feature_image(feature_image), original_image(original_image),
     current_block(cv::Rect(0, 0, block_width, block_height)), running(false),
     window_name(window_name)
  {
    ResetAnnotatedImage();
  }
};

static constexpr unsigned int border_size = 1;
static_assert(border_size < (block_width + 1) / 2, "Border size must be smaller than half the block width");
static_assert(border_size < (block_height + 1) / 2, "Border size must be smaller than half the block height");

static constexpr auto detection_threshold = 80000; //Feature-dependent threshold
static_assert(detection_threshold >= 1, "The detection threshold must be positive and non-zero");

static cv::Rect ExtendRect(const cv::Rect &rect, const unsigned int border)
{
  return cv::Rect(rect.x - border, rect.y - border, rect.width + 2 * border, rect.height + 2 * border);
}

static void HighlightBlock(cv::Mat &image, const cv::Rect &block, const cv::Scalar &color)
{
  const cv::Rect block_with_border = ExtendRect(block, border_size);
  rectangle(image, block_with_border, color, border_size);
}

static void OverlayImages(cv::Mat &original_image, const cv::Rect &image_block, const cv::Mat &overlay, const double alpha)
{
  assert(overlay.type() == CV_8UC1);
  cv::Mat image_region = original_image(image_block);
  cv::Mat converted_overlay;
  cv::cvtColor(overlay, converted_overlay, cv::COLOR_GRAY2BGRA);
  image_region = image_region * (1 - alpha) + converted_overlay * alpha;
}

static cv::Mat GetAnnotatedImage(haar_data &data, bool persist_changes)
{
  constexpr auto overlay_alpha = 0.33;
  if (persist_changes)
    HighlightBlock(data.annotated_image, data.current_block, imgutils::Green); //Draw detected faces in green and persist changes
  cv::Mat temporary_annotated_image = data.annotated_image.clone();
  if (!persist_changes)
    HighlightBlock(temporary_annotated_image, data.current_block, imgutils::Red); //Draw non-faces in red temporarily
  OverlayImages(temporary_annotated_image, data.current_block, data.feature_image, overlay_alpha);
  return temporary_annotated_image;
}

static double GetFeatureValue(const cv::Mat &block, const cv::Mat &feature)
{
  assert(block.size() == feature.size());
  assert(feature.type() == CV_8UC1);
  assert(feature.type() == block.type());
  
  cv::Mat block_signed;
  block.convertTo(block_signed, CV_16SC1);
  cv::Mat feature_signed;
  feature.convertTo(feature_signed, CV_16SC1, 1.0 / 127, -1); //Convert [0, 255] range to [0 / 127 - 1, 255 / 127 - 1] = [-1, 1]
  
  const cv::Mat weighted_pixels = block_signed.mul(feature_signed); //Multiply black pixels by (-1) and white pixels by 1
  const auto difference = sum(weighted_pixels); //Value is difference between weighted pixels
  const auto difference_1d = difference[0];
  return difference_1d;
}

static double UpdateImage(haar_data &data, const bool update_GUI = true)
{
  const cv::Mat block_pixels = data.original_image(data.current_block);
  const double feature_value = GetFeatureValue(block_pixels, data.feature_image);
  if (update_GUI)
  {
    const auto permanent_annotation = feature_value > detection_threshold;
    const cv::Mat annotated_image = GetAnnotatedImage(data, permanent_annotation);
    const std::string status_text = "Pixel difference (feature value): " + comutils::FormatValue(feature_value, 0);
    cv::displayStatusBar(data.window_name, status_text);
    cv::imshow(data.window_name, annotated_image);
  }
  return feature_value;
}

static double SetCurrentPosition(haar_data &data, const cv::Point &top_left, const bool update_GUI = true)
{
  data.current_block = cv::Rect(top_left, cv::Size(block_width, block_height));
  return UpdateImage(data, update_GUI);
}

static cv::Mat_<double> PerformSearch(haar_data &data, const bool update_GUI = true)
{
  constexpr auto search_step_delay = 1; //Animation delay in ms
  cv::Mat_<double> score_map(data.original_image.size(), std::numeric_limits<double>::infinity());
  for (int y = border_size; y <= data.original_image.cols - static_cast<int>(block_height + border_size); y++)
  {
    for (int x = border_size; x <= data.original_image.rows - static_cast<int>(block_width + border_size); x++)
    {
      if (update_GUI && !data.running) //Skip the rest when the user aborts
        return score_map;
      const cv::Point current_position(x, y);
      const double current_score = SetCurrentPosition(data, current_position, update_GUI);
      score_map(current_position - cv::Point(border_size, border_size)) = current_score;
      if (update_GUI)
        cv::waitKey(search_step_delay);
    }
  }
  return score_map;
}

static void ResetImage(haar_data &data)
{
  SetCurrentPosition(data, cv::Point(border_size, border_size)); //Set current position to (0, 0) plus border
}

static void SelectPointInImage(const int event, const int x, const int y, const int, void * const userdata)
{
  auto &data = *(static_cast<haar_data*>(userdata));
  if (!data.running && event == cv::EVENT_LBUTTONUP) //Only react when the left mouse button is being pressed while no motion estimation is running
  {
    const cv::Point mouse_point(x + border_size, y + border_size); //Assume mouse position is in the top-left of the search block (on the outside border which is border_size pixels in width)
    if (mouse_point.x <= data.original_image.cols - static_cast<int>(block_width + border_size) &&
        mouse_point.y <= data.original_image.rows - static_cast<int>(block_height + border_size)) //If the mouse is within the image area (minus the positions on the bottom which would lead to the block exceeding the borders)...
      SetCurrentPosition(data, mouse_point); //... set the current position according to the mouse position
  }
}

static cv::Mat MakeColorMap(const cv::Mat_<double> &diff_map)
{
  cv::Mat color_map(diff_map.size(), CV_8UC3);
  std::transform(diff_map.begin(), diff_map.end(), color_map.begin<cv::Vec3b>(),
                 [](const double value) -> cv::Vec3b
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

static void ShowImage(const cv::Mat &feature_image, const cv::Mat &image)
{
  constexpr auto window_name = "Image with objects to detect";
  cv::namedWindow(window_name);
  cv::moveWindow(window_name, 0, 0);
  static haar_data data(feature_image, image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  cv::setMouseCallback(window_name, SelectPointInImage, static_cast<void*>(&data));
  constexpr auto clear_button_name = "Clear detections";
  cv::createButton(clear_button_name, [](const int, void * const user_data)
                                        {
                                          auto &data = *(static_cast<haar_data*>(user_data));
                                          data.ResetAnnotatedImage();
                                          ResetImage(data); //Force redraw
                                        }, static_cast<void*>(&data), cv::QT_PUSH_BUTTON);
  constexpr auto perform_button_name = "Search whole image";
  cv::createButton(perform_button_name, [](const int, void * const user_data)
                                          {
                                            auto &data = *(static_cast<haar_data*>(user_data));
                                            if (!data.running)
                                            {
                                              data.running = true;
                                              PerformSearch(data);
                                              data.running = false;
                                            }
                                          }, static_cast<void*>(&data), cv::QT_PUSH_BUTTON);
  constexpr auto stop_button_name = "Stop search";
  cv::createButton(stop_button_name, [](const int, void * const user_data)
                                       {
                                         auto &data = *(static_cast<haar_data*>(user_data));
                                         data.running = false;
                                       }, static_cast<void*>(&data), cv::QT_PUSH_BUTTON);
  constexpr auto map_button_name = "Show map of differences";
  cv::createButton(map_button_name, [](const int, void * const user_data)
                                      {
                                        auto &data = *(static_cast<haar_data*>(user_data));
                                        if (data.running) //Abort when the search is already running
                                          return;
                                        auto diff_map = PerformSearch(data, false);
                                        const auto color_map = MakeColorMap(diff_map);
                                        constexpr auto map_window_name = "Difference map";
                                        cv::namedWindow(map_window_name);
                                        cv::moveWindow(map_window_name, data.original_image.cols + 3, 0); //Move difference window right of the main window (image size plus 3 border pixels)
                                        cv::imshow(map_window_name, color_map);
                                        cv::setMouseCallback(map_window_name, SelectPointInImage, static_cast<void*>(&data)); //Perform the same action as in the main window
                                      }, static_cast<void*>(&data), cv::QT_PUSH_BUTTON);
  ResetImage(data); //Implies cv::imshow with default position
}

static cv::Mat GetFeatureImage()
{
  cv::Mat feature_image(block_height, block_width, CV_8UC1, cv::Scalar(0));
  auto lower_half = feature_image(cv::Rect(0, block_height / 2, block_width, block_height / 2));
  lower_half = cv::Scalar(255);
  return feature_image;
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
  const cv::Mat feature_image = GetFeatureImage();
  if (image.rows < feature_image.rows + static_cast<int>(2 * border_size) && image.cols < feature_image.cols + static_cast<int>(2 * border_size))
  {
    std::cerr << "The input image must be larger than " << (feature_image.rows + 2 * border_size) << "x" << (feature_image.cols + 2 * border_size) << " pixels" << std::endl;
    return 3;
  }
  ShowImage(feature_image, image);
  cv::waitKey(0);
  return 0;
}
