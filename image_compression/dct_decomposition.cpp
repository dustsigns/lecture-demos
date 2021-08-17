//Illustration of the decomposition of a block into 2-D DCT basis functions and their recomposition
// Andreas Unterweger, 2017-2021
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

using namespace std;

using namespace cv;

using namespace comutils;
using namespace imgutils;

static constexpr unsigned int log_max_block_size = 6; //(1 << 6) = 64
static constexpr unsigned int max_block_size = 1 << log_max_block_size; //Maximum DCT size (2^log_max_block_size)

static constexpr unsigned int log_default_block_size = 3; //(1 << 3) = 8
static_assert(log_default_block_size <= log_max_block_size, "The default block size cannot be larger than the maximum block size");

struct DCT_data
{
  const Mat image;
  atomic_bool running;
  
  const string window_name;
  const string detail_window_name;
  const string sum_window_name;
  
  static constexpr auto trackbar_name = "log2(transform size)";
  
  DCT_data(const Mat &image, const string &window_name, const string &detail_window_name, const string &sum_window_name)
   : image(image), 
     running(false),
     window_name(window_name), detail_window_name(detail_window_name), sum_window_name(sum_window_name) { }
};

static Mat GetCenterBlock(const Mat &image, const unsigned int block_size)
{
  const Point center_point(image.rows / 2, image.cols / 2);
  const Rect center_rect(center_point, Size(block_size, block_size));
  const Mat center_block = image(center_rect);
  return center_block;
}

static Mat Decompose(const Mat &image, Mat_<double> &raw_coefficients)
{
  assert(image.cols == image.rows);
  const unsigned int block_size = static_cast<unsigned int>(image.rows);
  const Mat shifted_image = ImageLevelShift(image);
  dct(shifted_image, raw_coefficients);
  raw_coefficients.forEach([block_size](double &value, const int position[])
                                       {
                                         value *= Get2DDCTCoefficientScalingFactor(block_size, position[0], position[1]);
                                       });
  Mat decomposed_image = ReverseImageLevelShift(raw_coefficients);
  return decomposed_image;
}

static Mat HighlightCoefficient(const Mat &image, const unsigned int highlighted_x_index, const unsigned int highlighted_y_index)
{
  assert(image.type() == CV_8UC1);
  Mat image_highlighted;
  cvtColor(image, image_highlighted, COLOR_GRAY2BGR); //Convert image to color so that coefficients can be highlighted
  auto &pixel = image_highlighted.at<Vec3b>(highlighted_y_index, highlighted_x_index);
  pixel[0] = 0; //Set B zero
  pixel[1] = 0; //Set G zero => R remains, highlighting in red with the previous grayscale value
  return image_highlighted;
}

static double ShowImageAndDCT(const Mat &image, const unsigned int highlighted_x_index, const unsigned int highlighted_y_index, const string &window_name)
{
  Mat_<double> coefficients;
  const Mat decomposed_image = Decompose(image, coefficients);
  const Mat decomposed_image_highlighted = HighlightCoefficient(decomposed_image, highlighted_x_index, highlighted_y_index);
  const Mat combined_image = CombineImages({image, decomposed_image_highlighted}, Horizontal, 1);
  imshow(window_name, combined_image);
  return coefficients(highlighted_y_index, highlighted_x_index);
}

static Mat ShowWeightedBasisFunctionImage(const unsigned int x_index, const unsigned int y_index, const double value, unsigned int block_size, const string &window_name)
{
  const auto shifted_value = ReverseLevelShift(value);
  const auto sanitized_shifted_value = max(0.0, min(shifted_value, 255.0)); //Make sure the level is within the 8-bit range [0;255] in case of rounding and/or floating-point errors
  const Mat basis_function = Get2DDCTBasisFunctionImage(block_size, y_index, x_index);
  const Mat raw_weighted_basis_function = GetRaw2DDCTBasisFunctionImage(block_size, y_index, x_index, sanitized_shifted_value); //For calculating sums (not for illustration)
  const Mat weighted_basis_function = Get2DDCTBasisFunctionImage(block_size, y_index, x_index, sanitized_shifted_value);
  const Mat combined_image = CombineImages({basis_function, weighted_basis_function}, Horizontal, 1);
  const string status_text = "Coefficient (" + to_string(x_index) + ", " + to_string(y_index) + "): " + to_string(value);
  //displayOverlay(window_name, status_text, 1000);
  displayStatusBar(window_name, status_text);
  imshow(window_name, combined_image);
  return raw_weighted_basis_function;
}

static Mat SetFocusedCoefficient(const unsigned int x_index, const unsigned int y_index, const DCT_data &data, const int block_size)
{
  const Mat image_part = GetCenterBlock(data.image, block_size);
  const auto value = ShowImageAndDCT(image_part, x_index, y_index, data.window_name);
  const auto raw_weighted_basis_function_image = ShowWeightedBasisFunctionImage(x_index, y_index, value, block_size, data.detail_window_name);
  return raw_weighted_basis_function_image;
}

using coefficient_index = pair<size_t, size_t>;

static vector<coefficient_index> ZigZagScanIndices(unsigned int block_size)
{
  assert(block_size >= 1);
  size_t x = 0, y = 0;
  bool up = true;
  vector<coefficient_index> indices;
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
  Mat raw_sum(block_size, block_size, CV_64FC1, Scalar(0.0)); //Initialize sum with zeros
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
    const Mat sum = ReverseImageLevelShift(raw_sum);
    imshow(data.sum_window_name, sum);
    coefficient++;
    const string status_text = to_string(coefficient) + " of " + to_string(block_size * block_size) + " coefficients (" + FormatValue((100.0 * coefficient) / (block_size * block_size)) + "%)";
    displayStatusBar(data.sum_window_name, status_text);
    waitKey(max(1.0, (double)step_delay / coefficient)); //Move faster for higher-frequency coefficients (minimum 1 since a keypress would be required otherwise)
  }
}

static unsigned int GetBlockSize(const unsigned int log_block_size)
{
   return 1U << log_block_size; //2^(log_block_size)
}

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "DCT decomposition";
  namedWindow(window_name, WINDOW_NORMAL);
  constexpr auto detail_window_name = "Associated basis function";
  namedWindow(detail_window_name, WINDOW_NORMAL);
  constexpr auto sum_window_name = "Sum of weighted basis functions";
  namedWindow(sum_window_name, WINDOW_NORMAL);
  static DCT_data data(image, window_name, detail_window_name, sum_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  createTrackbar(data.trackbar_name, window_name, nullptr, log_max_block_size, [](const int log_block_size, void * const user_data)
                                                                                 {
                                                                                   auto &data = *(static_cast<DCT_data*>(user_data));
                                                                                   data.running = false; //Make sure the animation is stopped
                                                                                   const int block_size = GetBlockSize(log_block_size);
                                                                                   Mat empty_sum(block_size, block_size, CV_8UC1, Scalar(ReverseLevelShift(0))); //Create level-shifted zero image
                                                                                   imshow(data.sum_window_name, empty_sum); //Show all-grey image
                                                                                   displayStatusBar(data.sum_window_name, "Please start adding via the corresponding button.");
                                                                                   SetFocusedCoefficient(0, 0, data, block_size); //Set focus to DC coefficient (implies imshow)
                                                                                   constexpr auto displayed_window_size = 250;
                                                                                   resizeWindow(data.window_name, 2 * displayed_window_size, displayed_window_size);
                                                                                   moveWindow(data.window_name, 0, 0);
                                                                                   resizeWindow(data.detail_window_name, 2 * displayed_window_size, displayed_window_size);
                                                                                   moveWindow(data.detail_window_name, 2 * displayed_window_size + 3, 0); //Move basis function window right beside the DCT window (window width plus additional distance)
                                                                                   resizeWindow(data.sum_window_name, displayed_window_size, displayed_window_size);
                                                                                   moveWindow(data.sum_window_name, 2 * (2 * displayed_window_size + 3), 0); //Move basis function window right beside the DCT and basis function windows (two window widths plus additional distance, each)
                                                                                  }, static_cast<void*>(&data));
  setMouseCallback(window_name, [](const int event, const int x, const int y, const int, void * const userdata)
                                  {
                                    auto &data = *(static_cast<const DCT_data*>(userdata));
                                    if (event == EVENT_LBUTTONUP) //Only react when the left mouse button is being pressed
                                    {
                                      const auto log_block_size = getTrackbarPos(data.trackbar_name, data.window_name);
                                      const auto block_size = GetBlockSize(log_block_size);
                                      const int relative_x = x - block_size - 1; //Calculate relative coefficient position in image (original image and additional 1 pixel border)
                                      const int relative_y = y;
                                      if (relative_x >= 0 && static_cast<unsigned int>(relative_x) < block_size && relative_y >= 0 && static_cast<unsigned int>(relative_y) < block_size)
                                        SetFocusedCoefficient(relative_x, relative_y, data, block_size);
                                    }
                                  }, static_cast<void*>(&data));
  constexpr auto start_button_name = "Add weighted basis functions";
  createButton(start_button_name, [](const int, void * const user_data)
                                    {
                                      auto &data = *(static_cast<DCT_data*>(user_data));
                                      if (!data.running)
                                      {
                                        data.running = true;
                                        const auto log_block_size = getTrackbarPos(data.trackbar_name, data.window_name);
                                        const auto block_size = GetBlockSize(log_block_size);
                                        AddWeightedBasisFunctions(data, block_size);
                                        data.running = false;
                                      }
                                    }, static_cast<void*>(&data), QT_PUSH_BUTTON);
  constexpr auto stop_button_name = "Stop animation";
  createButton(stop_button_name, [](const int, void * const user_data)
                                   {
                                     auto &data = *(static_cast<DCT_data*>(user_data));
                                     data.running = false;
                                   }, static_cast<void*>(&data), QT_PUSH_BUTTON);
  setTrackbarPos(data.trackbar_name, window_name, log_default_block_size); //Implies imshow with DCT size of 8
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the DCT components of a block and their reassembly" << endl;
    cout << "Usage: " << argv[0] << " <input image>" << endl;
    return 1;
  }
  const auto image_path = argv[1];
  const Mat image = imread(image_path, IMREAD_GRAYSCALE);
  if (image.empty())
  {
	  cerr << "Could not read input image '" << image_path << "'" << endl;
	  return 2;
  }
  if ((unsigned int)min(image.rows, image.cols) < max_block_size)
  {
	  cerr << "The input image must be at least " << max_block_size << "x" << max_block_size << " pixels in size" << endl;
    return 3;
  }
  ShowImages(image);
  waitKey(0);
  return 0;
}
