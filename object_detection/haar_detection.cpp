//Illustration of Haar features for object detection
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <limits>
#include <atomic>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "format.hpp"
#include "colors.hpp"

using namespace std;

using namespace cv;

using namespace comutils;
using namespace imgutils;

static constexpr unsigned int block_width = 100;
static constexpr unsigned int block_height = 50;
static_assert(block_width % 2 == 0, "Block width must be even");
static_assert(block_height % 2 == 0, "Block height must be even");

typedef struct haar_data
{
  const Mat feature_image;
  const Mat original_image;
  Mat annotated_image;
  Rect current_block;
  atomic_bool running;
  
  const string window_name;
  
  void ResetAnnotatedImage()
  {
    cvtColor(original_image, annotated_image, COLOR_GRAY2BGRA);
  }
  
  haar_data(const Mat &feature_image, const Mat &original_image, const string &window_name)
   : feature_image(feature_image), original_image(original_image),
     current_block(Rect(0, 0, block_width, block_height)), running(false),
     window_name(window_name)
  {
    ResetAnnotatedImage();
  }
} haar_data;

static constexpr unsigned int border_size = 1;
static_assert(border_size < (block_width + 1) / 2, "Border size must be smaller than half the block width");
static_assert(border_size < (block_height + 1) / 2, "Border size must be smaller than half the block height");

static constexpr auto detection_threshold = 80000; //Feature-dependent threshold

static Rect ExtendRect(const Rect &rect, const unsigned int border)
{
  return Rect(rect.x - border, rect.y - border, rect.width + 2 * border, rect.height + 2 * border);
}

static void HighlightBlock(Mat &image, const Rect &block, const Scalar &color)
{
  const Rect block_with_border = ExtendRect(block, border_size);
  rectangle(image, block_with_border, color, border_size);
}

static void OverlayImages(Mat &original_image, const Rect &image_block, const Mat &overlay, const double alpha)
{
  assert(overlay.type() == CV_8UC1);
  Mat image_region = original_image(image_block);
  Mat converted_overlay;
  cvtColor(overlay, converted_overlay, COLOR_GRAY2BGRA);
  image_region = image_region * (1 - alpha) + converted_overlay * alpha;
}

static Mat GetAnnotatedImage(haar_data &data, bool persist_changes)
{
  constexpr auto overlay_alpha = 0.33;
  if (persist_changes)
    HighlightBlock(data.annotated_image, data.current_block, Green); //Draw detected faces in green and persist changes
  Mat temporary_annotated_image = data.annotated_image.clone();
  if (!persist_changes)
    HighlightBlock(temporary_annotated_image, data.current_block, Red); //Draw non-faces in red temporarily
  OverlayImages(temporary_annotated_image, data.current_block, data.feature_image, overlay_alpha);
  return temporary_annotated_image;
}

static double GetFeatureValue(const Mat &block, const Mat &feature)
{
  assert(block.size() == feature.size());
  assert(feature.type() == CV_8UC1);
  assert(feature.type() == block.type());
  
  Mat block_signed;
  block.convertTo(block_signed, CV_16SC1);
  Mat feature_signed;
  feature.convertTo(feature_signed, CV_16SC1, 1.0 / 127, -1); //Convert [0, 255] range to [0 / 127 - 1, 255 / 127 - 1] = [-1, 1]
  
  const Mat weighted_pixels = block_signed.mul(feature_signed); //Multiply black pixels by (-1) and white pixels by 1
  const auto difference = sum(weighted_pixels); //Value is difference between weighted pixels
  const auto difference_1d = difference[0];
  return difference_1d;
}

static double UpdateImage(haar_data &data)
{
  const Mat block_pixels = data.original_image(data.current_block);
  const double feature_value = GetFeatureValue(block_pixels, data.feature_image);
  const auto permanent_annotation = feature_value > detection_threshold;
  const Mat annotated_image = GetAnnotatedImage(data, permanent_annotation);
  const string status_text = "Pixel difference (feature value): " + FormatValue(feature_value, 0);
  displayStatusBar(data.window_name, status_text);
  imshow(data.window_name, annotated_image);
  return feature_value;
}

static double SetCurrentPosition(haar_data &data, const Point &top_left)
{
  data.current_block = Rect(top_left, Size(block_width, block_height));
  return UpdateImage(data);
}

static void PerformSearch(haar_data &data)
{
  constexpr auto search_step_delay = 1; //Animation delay in ms
  double highest_score = numeric_limits<double>::max();
  Point best_position;
  for (int y = border_size; y <= data.original_image.cols - static_cast<int>(block_height + border_size); y++)
  {
    for (int x = border_size; x <= data.original_image.rows - static_cast<int>(block_width + border_size); x++)
    {
      if (!data.running) //Skip the rest when the user aborts
        return;
      const Point current_position(x, y);
      const double current_score = SetCurrentPosition(data, current_position);
      waitKey(search_step_delay);
      highest_score = max(highest_score, current_score);
      if (current_score == highest_score) //Save the best position
        best_position = current_position;
    }
  }
  SetCurrentPosition(data, best_position);
} //TODO: Illustrate map of costs

static void ResetImage(haar_data &data)
{
  SetCurrentPosition(data, Point(border_size, border_size)); //Set current position to (0, 0) plus border
}

static void ShowImage(const Mat &feature_image, const Mat &image)
{
  constexpr auto window_name = "Image with objects to detect";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  static haar_data data(feature_image, image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  setMouseCallback(window_name, [](const int event, const int x, const int y, const int, void * const userdata)
                                  {
                                    auto &data = *((haar_data * const)userdata);
                                    if (!data.running && event == EVENT_LBUTTONUP) //Only react when the left mouse button is being pressed while no motion estimation is running
                                    {
                                      const Point mouse_point(x + border_size, y + border_size); //Assume mouse position is in the top-left of the search block (on the outside border which is border_size pixels in width)
                                      if (mouse_point.x <= data.original_image.cols - static_cast<int>(block_width + border_size) &&
                                          mouse_point.y <= data.original_image.rows - static_cast<int>(block_height + border_size)) //If the mouse is within the image area (minus the positions on the bottom which would lead to the block exceeding the borders)...
                                        SetCurrentPosition(data, mouse_point); //... set the current position according to the mouse position
                                    }
                                   }, (void*)&data);
  constexpr auto clear_button_name = "Clear detections";
  createButton(clear_button_name, [](const int, void * const user_data)
                                    {
                                      auto &data = *((haar_data * const)user_data);
                                      data.ResetAnnotatedImage();
                                      ResetImage(data); //Force redraw
                                    }, (void*)&data, QT_PUSH_BUTTON);
  constexpr auto perform_button_name = "Search whole image";
  createButton(perform_button_name, [](const int, void * const user_data)
                                      {
                                        auto &data = *((haar_data * const)user_data);
                                        if (!data.running)
                                        {
                                          data.running = true;
                                          PerformSearch(data);
                                          data.running = false;
                                        }
                                      }, (void*)&data, QT_PUSH_BUTTON);
  constexpr auto stop_button_name = "Stop search";
  createButton(stop_button_name, [](const int, void * const user_data)
                                   {
                                     auto &data = *((haar_data * const)user_data);
                                     data.running = false;
                                   }, (void*)&data, QT_PUSH_BUTTON);
  ResetImage(data); //Implies imshow with default position
}

static Mat GetFeatureImage()
{
  Mat feature_image(block_height, block_width, CV_8UC1, Scalar(0));
  auto lower_half = feature_image(Rect(0, block_height / 2, block_width, block_height / 2));
  lower_half = Scalar(255);
  return feature_image;
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates object detection with Haar features." << endl;
    cout << "Usage: " << argv[0] << " <input image>" << endl;
    return 1;
  }
  const auto image_filename = argv[1];
  const Mat image = imread(image_filename, IMREAD_GRAYSCALE);
  if (image.empty())
  {
    cerr << "Could not read input image '" << image_filename << "'" << endl;
    return 2;
  }
  const Mat feature_image = GetFeatureImage();
  if (image.rows < feature_image.rows + static_cast<int>(2 * border_size) && image.cols < feature_image.cols + static_cast<int>(2 * border_size))
  {
    cerr << "The input image must be larger than " << (feature_image.rows + 2 * border_size) << "x" << (feature_image.cols + 2 * border_size) << " pixels" << endl;
    return 3;
  }
  ShowImage(feature_image, image);
  waitKey(0);
  return 0;
}
