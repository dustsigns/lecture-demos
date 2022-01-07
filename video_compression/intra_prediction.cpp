//Illustration of intra prediction and the effect of residuals on transforms
// Andreas Unterweger, 2017-2022
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

static constexpr auto block_size = 32;

static constexpr auto region_size = 2 * block_size;
static_assert(region_size / 2 == block_size, "The region size must be even and equal to double the block size");

struct prediction_data
{
  const cv::Mat image;

  const std::string original_window_name;
  const std::string predicted_window_name;
  const std::string transformed_window_name;
  const std::string predicted_transformed_window_name;
  const std::string prediction_window_name;
  
  prediction_data(const cv::Mat &image, const std::string &original_window_name, const std::string &predicted_window_name, const std::string &transformed_window_name, const std::string &predicted_transformed_window_name, const std::string &prediction_window_name)
   : image(image),
     original_window_name(original_window_name), predicted_window_name(predicted_window_name), transformed_window_name(transformed_window_name), predicted_transformed_window_name(predicted_transformed_window_name), prediction_window_name(prediction_window_name) { }
};

using prediction_function = cv::Mat (*)(const cv::Mat&, const cv::Mat&);

struct prediction_function_data
{
  const std::string name;
  const prediction_data &data;
  const prediction_function function;
  const prediction_function illustration_function;
  
  prediction_function_data(const std::string &name, const prediction_data &data, const prediction_function function, const prediction_function illustration_function)
   : name(name), data(data), function(function), illustration_function(illustration_function) {}
};

static cv::Mat GetCenterRegion(const cv::Mat &image)
{
  const cv::Point center_point(image.rows / 2, image.cols / 2);
  const cv::Rect center_rect(center_point, cv::Size(region_size, region_size));
  const cv::Mat center_region = image(center_rect);
  return center_region;
}

static void SplitRegion(const cv::Mat &region, std::array<cv::Mat, 4> &blocks)
{
  assert(region.rows == region.cols);
  assert(region.rows % 2 == 0);
  
  const auto size = region.rows / 2;
  blocks[0] = region(cv::Rect(0, 0, size, size)); //Top left
  blocks[1] = region(cv::Rect(size, 0, size, size)); //Top right
  blocks[2] = region(cv::Rect(0, size, size, size)); //Bottom left
  blocks[3] = region(cv::Rect(size, size, size, size)); //Bottom right
}

static const cv::Mat MergeRegion(const std::array<cv::Mat, 4> &blocks)
{
  const auto size = blocks[0].rows;
  for (const auto &block : blocks)
  {
    assert(block.rows == block.cols);
    assert(block.rows == size);
  }
  
  cv::Mat region(2 * size, 2 * size, blocks[0].type());
  region(cv::Rect(0, 0, size, size)) = 1*blocks[0]; //Top left //TODO: Get rid of hack for assignment of temporary objects
  region(cv::Rect(size, 0, size, size)) = 1*blocks[1]; //Top right
  region(cv::Rect(0, size, size, size)) = 1*blocks[2]; //Bottom left
  region(cv::Rect(size, size, size, size)) = 1*blocks[3]; //Bottom right
  return region;
}

static cv::Mat PredictHorizontal(const cv::Mat&, const cv::Mat &bottom_left_block)
{
  assert(bottom_left_block.type() == CV_8UC1);
  auto &input_block = static_cast<const cv::Mat_<unsigned char>>(bottom_left_block);
  cv::Mat_<unsigned char> predicted_block(input_block.rows, input_block.cols);
  for (int y = 0; y < input_block.rows; y++)
  {
    const int last_pixel_position = input_block.cols - 1;
    const auto row_value = input_block[y][last_pixel_position]; //Use right-most pixel from the input block as a basis for prediction
    predicted_block.row(y).setTo(row_value); //Use the prediction value for the whole row
  }
  return predicted_block;
}

static cv::Mat DrawHorizontalArrow(const cv::Mat &top_right_block, const cv::Mat &bottom_left_block)
{
  assert(top_right_block.type() == bottom_left_block.type());
  assert(bottom_left_block.rows == top_right_block.cols);
  cv::Mat predicted_block(bottom_left_block.rows, top_right_block.cols, top_right_block.type(), cv::Scalar(0));
  
  predicted_block.col(0).setTo(imgutils::Red);
  const cv::Point middle_left(0, bottom_left_block.rows / 2);
  const cv::Point middle_right(top_right_block.cols - 1, bottom_left_block.rows / 2);
  arrowedLine(predicted_block, middle_left, middle_right, imgutils::Red);
  return predicted_block;
}

static cv::Mat PredictVertical(const cv::Mat &top_right_block, const cv::Mat&)
{
  assert(top_right_block.type() == CV_8UC1);
  auto &input_block = static_cast<const cv::Mat_<unsigned char>>(top_right_block);
  cv::Mat_<unsigned char> predicted_block(input_block.rows, input_block.cols);
  for (int x = 0; x < input_block.cols; x++)
  {
    const int last_pixel_position = input_block.rows - 1;
    const auto col_value = input_block[last_pixel_position][x]; //Use bottom-most pixel from the input block as a basis for prediction
    predicted_block.col(x).setTo(col_value); //Use the prediction value for the whole column
  }
  return predicted_block;
}

static cv::Mat DrawVerticalArrow(const cv::Mat &top_right_block, const cv::Mat &bottom_left_block)
{
  assert(top_right_block.type() == bottom_left_block.type());
  assert(bottom_left_block.rows == top_right_block.cols);
  cv::Mat predicted_block(bottom_left_block.rows, top_right_block.cols, top_right_block.type(), cv::Scalar(0));
  
  predicted_block.row(0).setTo(imgutils::Red);
  const cv::Point top_center(top_right_block.cols / 2, 0);
  const cv::Point bottom_center(top_right_block.cols / 2, bottom_left_block.rows - 1);
  arrowedLine(predicted_block, top_center, bottom_center, imgutils::Red);
  return predicted_block;
}

static cv::Mat PredictRegion(const cv::Mat &image, const prediction_function prediction_function_pointer, cv::Mat &original_block, cv::Mat &predicted_block)
{
  std::array<cv::Mat, 4> blocks;
  SplitRegion(image, blocks);
  original_block = blocks[3]; //Bottom-right block
  predicted_block = (*prediction_function_pointer)(blocks[1], blocks[2]); //Predict from top-right and bottom-left block
  blocks[3] = predicted_block;
  const cv::Mat predicted_image = MergeRegion(blocks);
  return predicted_image;
}

static void ShowPrediction(const cv::Mat &image, const std::string &window_name, const prediction_function prediction_function_pointer, cv::Mat &original_block, cv::Mat &predicted_block)
{
  const cv::Mat predicted_region = PredictRegion(image, prediction_function_pointer, original_block, predicted_block);
  cv::imshow(window_name, predicted_region);
}

static void ShowPredictionIllustration(const cv::Mat &image, const std::string &window_name, const prediction_function prediction_function_pointer)
{
  cv::Mat color_image;
  cv::cvtColor(image, color_image, cv::COLOR_GRAY2BGR);
  cv::Mat original_block, predicted_block;
  const cv::Mat colored_predicted_region = PredictRegion(color_image, prediction_function_pointer, original_block, predicted_block);
  cv::imshow(window_name, colored_predicted_region);
}

static cv::Mat Decompose(const cv::Mat &image, cv::Mat_<double> &raw_coefficients)
{
  assert(image.cols == image.rows);
  assert(image.cols == block_size);
  const cv::Mat shifted_image = imgutils::ImageLevelShift(image);
  dct(shifted_image, raw_coefficients);
  raw_coefficients.forEach([](double &value, const int position[])
                             {
                               value *= comutils::Get2DDCTCoefficientScalingFactor(block_size, position[0], position[1]);
                             });
  cv::Mat decomposed_image = imgutils::ReverseImageLevelShift(raw_coefficients);
  return decomposed_image;
}

static double PercentageOfSmallCoefficients(const cv::Mat_<double> &input)
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

static void ShowDifferenceAndDCT(const cv::Mat &image, const std::string &window_name, bool image_is_difference = false)
{
  const cv::Mat difference_image = image_is_difference ? imgutils::ConvertDifferenceImage(image) : image;
  const cv::Mat dct_input_image = image_is_difference ? imgutils::ConvertDifferenceImage(image, imgutils::DifferenceConversionMode::Offset) : image;
  cv::Mat_<double> raw_coefficients;
  const cv::Mat decomposed_image = Decompose(dct_input_image, raw_coefficients);
  const cv::Mat combined_image = imgutils::CombineImages({difference_image, decomposed_image}, imgutils::CombinationMode::Horizontal, 1);
  cv::imshow(window_name, combined_image);
  const double YSAD = imgutils::SAD(difference_image);
  const double YSATD = imgutils::SAD(raw_coefficients);
  const auto percentage_small_coefficients = PercentageOfSmallCoefficients(raw_coefficients);
  const std::string status_text = "SAD: " + comutils::FormatValue(YSAD, 0) + ", SATD: " + comutils::FormatValue(YSATD) + ", small coefficients (|coefficient| < 1): " + comutils::FormatValue(percentage_small_coefficients) + "%";
  cv::displayStatusBar(window_name, status_text);
}

static void AlignWindows(const prediction_data &data)
{
  constexpr auto zoom = 7.5;
  cv::resizeWindow(data.original_window_name, region_size * zoom, region_size * zoom); //x zoom (image shows one region)
  cv::moveWindow(data.original_window_name, 0, 0);
  cv::resizeWindow(data.predicted_window_name, region_size * zoom, region_size * zoom); //x zoom (image shows one block)
  cv::moveWindow(data.predicted_window_name, region_size * zoom + 3, 0); //Move window with predicted image right beside the window with the original image (one region with zoom plus additional distance)
  cv::resizeWindow(data.prediction_window_name, region_size * zoom, region_size * zoom); //x zoom (image shows one block)
  cv::moveWindow(data.prediction_window_name, 2 * (region_size * zoom + 3), 0); //Move window with prediction illustration right beside the window with the prediction (one region with zoom plus additional distance, each)
  cv::resizeWindow(data.transformed_window_name, (2 * block_size + 1) * zoom, block_size * zoom); //x zoom (image shows on two blocks with one border pixel)
  cv::moveWindow(data.transformed_window_name, 0, region_size * zoom + 50); //Move window with DCT right below the corresponding image additional distance
  cv::resizeWindow(data.predicted_transformed_window_name, (2 * block_size + 1) * zoom, block_size * zoom); //x zoom (image shows on two blocks with one border pixel)
  cv::moveWindow(data.predicted_transformed_window_name, region_size * zoom + 3, region_size * zoom + 50); //Move window with DCT right beside the window with the original image (one region with zoom plus additional distance) and below the corresponding image plus additional distance
}

static void ShowImages(const cv::Mat &region)
{
  constexpr auto original_window_name = "Original";
  cv::namedWindow(original_window_name, cv::WINDOW_NORMAL);
  constexpr auto predicted_window_name = "Predicted";
  cv::namedWindow(predicted_window_name, cv::WINDOW_NORMAL);
  constexpr auto transformed_window_name = "Original and its DCT";
  cv::namedWindow(transformed_window_name, cv::WINDOW_NORMAL);
  constexpr auto predicted_transformed_window_name = "Residual and its DCT";
  cv::namedWindow(predicted_transformed_window_name, cv::WINDOW_NORMAL);
  constexpr auto prediction_window_name = "Prediction illustration";
  cv::namedWindow(prediction_window_name, cv::WINDOW_NORMAL);
  static prediction_data data(region, original_window_name, predicted_window_name, transformed_window_name, predicted_transformed_window_name, prediction_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  static const prediction_function_data prediction_methods[] {{"Horizontal", data, PredictHorizontal, DrawHorizontalArrow},
                                                              {"Vertical", data, PredictVertical, DrawVerticalArrow}}; //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  for (const auto &prediction_method : prediction_methods)
  {
    cv::createButton(prediction_method.name, [](const int state, void * const user_data)
                                               {
                                                 if (!state) //Ignore radio button events where the button becomes unchecked
                                                   return;
                                                cv::imshow(data.original_window_name, data.image); //Show original image
                                                auto &prediction_method = *(static_cast<const prediction_function_data*>(user_data));
                                                auto &data = prediction_method.data;
                                                cv::Mat original_block, predicted_block;
                                                ShowPrediction(data.image, data.predicted_window_name, prediction_method.function, original_block, predicted_block);
                                                ShowDifferenceAndDCT(original_block, data.transformed_window_name);
                                                const cv::Mat difference = imgutils::SubtractImages(original_block, predicted_block);
                                                ShowDifferenceAndDCT(difference, data.predicted_transformed_window_name, true);
                                                ShowPredictionIllustration(data.image, data.prediction_window_name, prediction_method.illustration_function);
                                              }, const_cast<void*>(static_cast<const void*>(&prediction_method)), cv::QT_RADIOBOX, &prediction_method == &prediction_methods[comutils::arraysize(prediction_methods) - 1]); //Make last radio button checked
  }
  AlignWindows(data);
  //The first radio button is checked by default, triggering an update event implying cv::imshow
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates intra prediction and its effect on the subsequent transform" << std::endl;
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
  if (static_cast<unsigned int>(std::min(image.rows, image.cols)) < region_size)
  {
	  std::cerr << "The input image must be at least " << region_size << "x" << region_size << " pixels in size" << std::endl;
    return 3;
  }
  const cv::Mat region = GetCenterRegion(image);
  ShowImages(region);
  cv::waitKey(0);
  return 0;
}
