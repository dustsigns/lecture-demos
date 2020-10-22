//Illustration of intra prediction and the effect of residuals on transforms
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <array>
#include <cassert>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "common.hpp"
#include "math.hpp"
#include "imgmath.hpp"
#include "combine.hpp"
#include "format.hpp"
#include "colors.hpp"

using namespace std;

using namespace cv;

using namespace comutils;
using namespace imgutils;

static constexpr auto block_size = 32;

static constexpr auto region_size = 2 * block_size;
static_assert(region_size / 2 == block_size, "The region size must be even and equal to double the block size");

struct prediction_data
{
  const Mat image;

  const string original_window_name;
  const string predicted_window_name;
  const string transformed_window_name;
  const string predicted_transformed_window_name;
  const string prediction_window_name;
  
  prediction_data(const Mat &image, const string &original_window_name, const string &predicted_window_name, const string &transformed_window_name, const string &predicted_transformed_window_name, const string &prediction_window_name)
   : image(image),
     original_window_name(original_window_name), predicted_window_name(predicted_window_name), transformed_window_name(transformed_window_name), predicted_transformed_window_name(predicted_transformed_window_name), prediction_window_name(prediction_window_name) { }
};

using prediction_function = Mat (*)(const Mat&, const Mat&);

struct prediction_function_data
{
  const string name;
  const prediction_data &data;
  const prediction_function function;
  const prediction_function illustration_function;
  
  prediction_function_data(const string &name, const prediction_data &data, const prediction_function function, const prediction_function illustration_function)
   : name(name), data(data), function(function), illustration_function(illustration_function) {}
};

static Mat GetCenterRegion(const Mat &image)
{
  const Point center_point(image.rows / 2, image.cols / 2);
  const Rect center_rect(center_point, Size(region_size, region_size));
  const Mat center_region = image(center_rect);
  return center_region;
}

static void SplitRegion(const Mat &region, array<Mat, 4> &blocks)
{
  assert(region.rows == region.cols);
  assert(region.rows % 2 == 0);
  
  const auto size = region.rows / 2;
  blocks[0] = region(Rect(0, 0, size, size)); //Top left
  blocks[1] = region(Rect(size, 0, size, size)); //Top right
  blocks[2] = region(Rect(0, size, size, size)); //Bottom left
  blocks[3] = region(Rect(size, size, size, size)); //Bottom right
}

static const Mat MergeRegion(const array<Mat, 4> &blocks)
{
  const auto size = blocks[0].rows;
  for (const auto &block : blocks)
  {
    assert(block.rows == block.cols);
    assert(block.rows == size);
  }
  
  Mat region(2 * size, 2 * size, blocks[0].type());
  region(Rect(0, 0, size, size)) = 1*blocks[0]; //Top left //TODO: Get rid of hack for assignment of temporary objects
  region(Rect(size, 0, size, size)) = 1*blocks[1]; //Top right
  region(Rect(0, size, size, size)) = 1*blocks[2]; //Bottom left
  region(Rect(size, size, size, size)) = 1*blocks[3]; //Bottom right
  return region;
}

static Mat PredictHorizontal(const Mat&, const Mat &bottom_left_block)
{
  assert(bottom_left_block.type() == CV_8UC1);
  auto &input_block = static_cast<const Mat_<unsigned char>>(bottom_left_block);
  Mat_<unsigned char> predicted_block(input_block.rows, input_block.cols);
  for (int y = 0; y < input_block.rows; y++)
  {
    const int last_pixel_position = input_block.cols - 1;
    const auto row_value = input_block[y][last_pixel_position]; //Use right-most pixel from the input block as a basis for prediction
    predicted_block.row(y).setTo(row_value); //Use the prediction value for the whole row
  }
  return predicted_block;
}

static Mat DrawHorizontalArrow(const Mat &top_right_block, const Mat &bottom_left_block)
{
  assert(top_right_block.type() == bottom_left_block.type());
  assert(bottom_left_block.rows == top_right_block.cols);
  Mat predicted_block(bottom_left_block.rows, top_right_block.cols, top_right_block.type(), Scalar(0));
  
  predicted_block.col(0).setTo(Red);
  const Point middle_left(0, bottom_left_block.rows / 2);
  const Point middle_right(top_right_block.cols - 1, bottom_left_block.rows / 2);
  arrowedLine(predicted_block, middle_left, middle_right, Red);
  return predicted_block;
}

static Mat PredictVertical(const Mat &top_right_block, const Mat&)
{
  assert(top_right_block.type() == CV_8UC1);
  auto &input_block = static_cast<const Mat_<unsigned char>>(top_right_block);
  Mat_<unsigned char> predicted_block(input_block.rows, input_block.cols);
  for (int x = 0; x < input_block.cols; x++)
  {
    const int last_pixel_position = input_block.rows - 1;
    const auto col_value = input_block[last_pixel_position][x]; //Use bottom-most pixel from the input block as a basis for prediction
    predicted_block.col(x).setTo(col_value); //Use the prediction value for the whole column
  }
  return predicted_block;
}

static Mat DrawVerticalArrow(const Mat &top_right_block, const Mat &bottom_left_block)
{
  assert(top_right_block.type() == bottom_left_block.type());
  assert(bottom_left_block.rows == top_right_block.cols);
  Mat predicted_block(bottom_left_block.rows, top_right_block.cols, top_right_block.type(), Scalar(0));
  
  predicted_block.row(0).setTo(Red);
  const Point top_center(top_right_block.cols / 2, 0);
  const Point bottom_center(top_right_block.cols / 2, bottom_left_block.rows - 1);
  arrowedLine(predicted_block, top_center, bottom_center, Red);
  return predicted_block;
}

static Mat PredictRegion(const Mat &image, const prediction_function prediction_function_pointer, Mat &original_block, Mat &predicted_block)
{
  array<Mat, 4> blocks;
  SplitRegion(image, blocks);
  original_block = blocks[3]; //Bottom-right block
  predicted_block = (*prediction_function_pointer)(blocks[1], blocks[2]); //Predict from top-right and bottom-left block
  blocks[3] = predicted_block;
  const Mat predicted_image = MergeRegion(blocks);
  return predicted_image;
}

static void ShowPrediction(const Mat &image, const string &window_name, const prediction_function prediction_function_pointer, Mat &original_block, Mat &predicted_block)
{
  const Mat predicted_region = PredictRegion(image, prediction_function_pointer, original_block, predicted_block);
  imshow(window_name, predicted_region);
}

static void ShowPredictionIllustration(const Mat &image, const string &window_name, const prediction_function prediction_function_pointer)
{
  Mat color_image;
  cvtColor(image, color_image, COLOR_GRAY2BGR);
  Mat original_block, predicted_block;
  const Mat colored_predicted_region = PredictRegion(color_image, prediction_function_pointer, original_block, predicted_block);
  imshow(window_name, colored_predicted_region);
}

static Mat Decompose(const Mat &image, Mat_<double> &raw_coefficients)
{
  assert(image.cols == image.rows);
  assert(image.cols == block_size);
  const Mat shifted_image = ImageLevelShift(image);
  dct(shifted_image, raw_coefficients);
  raw_coefficients.forEach([](double &value, const int position[])
                             {
                               value *= Get2DDCTCoefficientScalingFactor(block_size, position[0], position[1]);
                             });
  Mat decomposed_image = ReverseImageLevelShift(raw_coefficients);
  return decomposed_image;
}

static double PercentageOfSmallCoefficients(const Mat_<double> &input)
{
  constexpr double absolute_threshhold = 1;
  unsigned int count = 0;
  for (const auto &coefficient : input)
  {
    if (fabs(coefficient) < absolute_threshhold)
      count++;
  }
  const auto percentage_small_coefficients = (count * 100.0) / input.total();
  return percentage_small_coefficients;
}

static void ShowDifferenceAndDCT(const Mat &image, const string &window_name, bool image_is_difference = false)
{
  const Mat difference_image = image_is_difference ? ConvertDifferenceImage(image) : image;
  const Mat dct_input_image = image_is_difference ? ConvertDifferenceImage(image, Offset) : image;
  Mat_<double> raw_coefficients;
  const Mat decomposed_image = Decompose(dct_input_image, raw_coefficients);
  const Mat combined_image = CombineImages({difference_image, decomposed_image}, Horizontal, 1);
  imshow(window_name, combined_image);
  const double YSAD = SAD(difference_image);
  const double YSATD = SAD(raw_coefficients);
  const auto percentage_small_coefficients = PercentageOfSmallCoefficients(raw_coefficients);
  const string status_text = "SAD: " + FormatValue(YSAD, 0) + ", SATD: " + FormatValue(YSATD) + ", small coefficients (|coefficient| < 1): " + FormatValue(percentage_small_coefficients) + "%";
  displayStatusBar(window_name, status_text);
}

static void AlignWindows(const prediction_data &data)
{
  constexpr auto zoom = 7.5;
  resizeWindow(data.original_window_name, region_size * zoom, region_size * zoom); //x zoom (image shows one region)
  moveWindow(data.original_window_name, 0, 0);
  resizeWindow(data.predicted_window_name, region_size * zoom, region_size * zoom); //x zoom (image shows one block)
  moveWindow(data.predicted_window_name, region_size * zoom + 3, 0); //Move window with predicted image right beside the window with the original image (one region with zoom plus additional distance)
  resizeWindow(data.prediction_window_name, region_size * zoom, region_size * zoom); //x zoom (image shows one block)
  moveWindow(data.prediction_window_name, 2 * (region_size * zoom + 3), 0); //Move window with prediction illustration right beside the window with the prediction (one region with zoom plus additional distance, each)
  resizeWindow(data.transformed_window_name, (2 * block_size + 1) * zoom, block_size * zoom); //x zoom (image shows on two blocks with one border pixel)
  moveWindow(data.transformed_window_name, 0, region_size * zoom + 50); //Move window with DCT right below the corresponding image additional distance
  resizeWindow(data.predicted_transformed_window_name, (2 * block_size + 1) * zoom, block_size * zoom); //x zoom (image shows on two blocks with one border pixel)
  moveWindow(data.predicted_transformed_window_name, region_size * zoom + 3, region_size * zoom + 50); //Move window with DCT right beside the window with the original image (one region with zoom plus additional distance) and below the corresponding image plus additional distance
}

static void ShowImages(const Mat &region)
{
  constexpr auto original_window_name = "Original";
  namedWindow(original_window_name, WINDOW_NORMAL);
  constexpr auto predicted_window_name = "Predicted";
  namedWindow(predicted_window_name, WINDOW_NORMAL);
  constexpr auto transformed_window_name = "Original and its DCT";
  namedWindow(transformed_window_name, WINDOW_NORMAL);
  constexpr auto predicted_transformed_window_name = "Residual and its DCT";
  namedWindow(predicted_transformed_window_name, WINDOW_NORMAL);
  constexpr auto prediction_window_name = "Prediction illustration";
  namedWindow(prediction_window_name, WINDOW_NORMAL);
  static prediction_data data(region, original_window_name, predicted_window_name, transformed_window_name, predicted_transformed_window_name, prediction_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  static const prediction_function_data prediction_methods[] {{"Horizontal", data, PredictHorizontal, DrawHorizontalArrow},
                                                              {"Vertical", data, PredictVertical, DrawVerticalArrow}}; //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  for (const auto &prediction_method : prediction_methods)
  {
    createButton(prediction_method.name, [](const int state, void * const user_data)
                                           {
                                             if (!state) //Ignore radio button events where the button becomes unchecked
                                               return;
                                            imshow(data.original_window_name, data.image); //Show original image
                                            auto &prediction_method = *(static_cast<const prediction_function_data*>(user_data));
                                            auto &data = prediction_method.data;
                                            Mat original_block, predicted_block;
                                            ShowPrediction(data.image, data.predicted_window_name, prediction_method.function, original_block, predicted_block);
                                            ShowDifferenceAndDCT(original_block, data.transformed_window_name);
                                            const Mat difference = SubtractImages(original_block, predicted_block);
                                            ShowDifferenceAndDCT(difference, data.predicted_transformed_window_name, true);
                                            ShowPredictionIllustration(data.image, data.prediction_window_name, prediction_method.illustration_function);
                                          }, const_cast<void*>(static_cast<const void*>(&prediction_method)), QT_RADIOBOX, &prediction_method == &prediction_methods[arraysize(prediction_methods) - 1]); //Make last radio button checked
  }
  AlignWindows(data);
  //The first radio button is checked by default, triggering an update event implying imshow
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates intra prediction and its effect on the subsequent transform" << endl;
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
  if (static_cast<unsigned int>(min(image.rows, image.cols)) < region_size)
  {
	  cerr << "The input image must be at least " << region_size << "x" << region_size << " pixels in size" << endl;
    return 3;
  }
  const Mat region = GetCenterRegion(image);
  ShowImages(region);
  waitKey(0);
  return 0;
}
