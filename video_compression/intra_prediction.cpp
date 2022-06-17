//Illustration of intra prediction and the effect of residuals on transforms
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <array>
#include <cassert>
#include <memory>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "common.hpp"
#include "math.hpp"
#include "imgmath.hpp"
#include "combine.hpp"
#include "format.hpp"
#include "colors.hpp"
#include "window.hpp"
#include "multiwin.hpp"

struct prediction_function_data
{
  using prediction_function = cv::Mat(const cv::Mat&, const cv::Mat&);
  
  const char * const name; //TODO: Make std::string, also in constructor (requires C++20)
  prediction_function * const function;
  prediction_function * const illustration_function;
  
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
  
  constexpr prediction_function_data(const char * const name, prediction_function * const function, prediction_function * const illustration_function)
   : name(name), function(function), illustration_function(illustration_function) { }
};

class prediction_data
{
  public:
    static constexpr auto block_size = 32;

    static constexpr auto region_size = 2 * block_size;
    static_assert(region_size / 2 == block_size, "The region size must be even and equal to double the block size");
  protected:
    static constexpr prediction_function_data prediction_methods[] {{"Horizontal", prediction_function_data::PredictHorizontal, prediction_function_data::DrawHorizontalArrow},
                                                                    {"Vertical", prediction_function_data::PredictVertical, prediction_function_data::DrawVerticalArrow}};
    static constexpr auto &default_prediction_method = prediction_methods[1]; //Vertical by default
  
    imgutils::Window original_window;
    
    using RadioButtonType = imgutils::RadioButton<prediction_data&, const prediction_function_data&>;
    std::unique_ptr<RadioButtonType> prediction_method_radiobuttons[comutils::arraysize(prediction_methods)];
    
    imgutils::Window transformed_window;
    
    imgutils::Window predicted_window;
    
    imgutils::Window predicted_transformed_window;
    
    imgutils::Window prediction_window;
    
    imgutils::MultiWindow original_and_transformed_windows;
    imgutils::MultiWindow predicted_and_transformed_windows;
    imgutils::MultiWindow all_windows;
  
    const cv::Mat region;
    
    static cv::Mat GetCenterRegion(const cv::Mat &image)
    {
      const cv::Point center_point(image.rows / 2, image.cols / 2);
      const cv::Rect center_rect(center_point, cv::Size(region_size, region_size));
      const cv::Mat center_region = image(center_rect);
      return center_region;
    }
    
    void ShowOriginal(const unsigned int zoom_factor)
    {
      original_window.UpdateContent(region);
      original_window.Zoom(zoom_factor);
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

    static cv::Mat PredictRegion(const cv::Mat &region, prediction_function_data::prediction_function * const prediction_function_pointer, cv::Mat &original_block, cv::Mat &predicted_block)
    {
      std::array<cv::Mat, 4> blocks;
      SplitRegion(region, blocks);
      original_block = blocks[3]; //Bottom-right block
      predicted_block = (*prediction_function_pointer)(blocks[1], blocks[2]); //Predict from top-right and bottom-left block
      blocks[3] = predicted_block;
      const cv::Mat predicted_image = MergeRegion(blocks);
      return predicted_image;
    }

    static void ShowPrediction(const cv::Mat &region, imgutils::Window &window, prediction_function_data::prediction_function * const prediction_function_pointer, cv::Mat &original_block, cv::Mat &predicted_block, const unsigned int zoom_factor)
    {
      const cv::Mat predicted_region = PredictRegion(region, prediction_function_pointer, original_block, predicted_block);
      window.UpdateContent(predicted_region);
      window.Zoom(zoom_factor);
    }

    void ShowPredictionIllustration(prediction_function_data::prediction_function * const prediction_function_pointer, const unsigned int zoom_factor)
    {
      cv::Mat color_region;
      cv::cvtColor(region, color_region, cv::COLOR_GRAY2BGR);
      cv::Mat original_block, predicted_block;
      const cv::Mat colored_predicted_region = PredictRegion(color_region, prediction_function_pointer, original_block, predicted_block);
      prediction_window.UpdateContent(colored_predicted_region);
      prediction_window.Zoom(zoom_factor);
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

    static double PercentageOfSmallCoefficients(const cv::Mat_<double> &input, const double absolute_threshhold)
    {
      unsigned int count = 0;
      for (const auto &coefficient : input)
      {
        if (fabs(coefficient) < absolute_threshhold)
          count++;
      }
      const auto percentage_small_coefficients = (count * 100.0) / input.total();
      return percentage_small_coefficients;
    }

    static void ShowDifferenceAndDCT(const cv::Mat image, imgutils::Window &window, const unsigned int zoom_factor, bool image_is_difference = false)
    {
      constexpr auto absolute_threshhold = 5.0;
      
      const cv::Mat difference_image = image_is_difference ? imgutils::ConvertDifferenceImage(image) : image;
      const cv::Mat dct_input_image = image_is_difference ? imgutils::ConvertDifferenceImage(image, imgutils::DifferenceConversionMode::Offset) : image;
      cv::Mat_<double> raw_coefficients;
      const cv::Mat decomposed_image = Decompose(dct_input_image, raw_coefficients);
      const cv::Mat combined_image = imgutils::CombineImages({difference_image, decomposed_image}, imgutils::CombinationMode::Horizontal, 1);
      window.UpdateContent(combined_image);
      window.Zoom(zoom_factor);
      const double YSAD = imgutils::SAD(difference_image);
      const double YSATD = imgutils::SAD(raw_coefficients);
      const auto percentage_small_coefficients = PercentageOfSmallCoefficients(raw_coefficients, absolute_threshhold);
      if (window.IsShown())
      {
        const std::string status_text = "SAD: " + comutils::FormatValue(YSAD, 0) + ", SATD: " + comutils::FormatValue(YSATD) + ", small coefficients (|coeff.| < " + comutils::FormatValue(absolute_threshhold, 0) + "): " + comutils::FormatValue(percentage_small_coefficients) + "%";
        window.ShowOverlayText(status_text, true);
      }
    }

    static void UpdateImages(prediction_data &data, const prediction_function_data &prediction_method)
    {
      constexpr auto zoom_factor = 7.5;
      cv::Mat original_block, predicted_block;
      data.ShowOriginal(zoom_factor);
      ShowPrediction(data.region, data.predicted_window, prediction_method.function, original_block, predicted_block, zoom_factor);
      ShowDifferenceAndDCT(original_block, data.transformed_window, zoom_factor);
      const cv::Mat difference = imgutils::SubtractImages(original_block, predicted_block);
      ShowDifferenceAndDCT(difference, data.predicted_transformed_window, zoom_factor, true);
      data.ShowPredictionIllustration(prediction_method.illustration_function, zoom_factor);
    }

    void AddRadioButtons()
    {
      std::transform(std::begin(prediction_methods), std::end(prediction_methods), std::begin(prediction_method_radiobuttons),
                                [this](const prediction_function_data &prediction_method)
                                      {
                                        const auto radiobutton_name = prediction_method.name;
                                        const auto default_checked = &prediction_method == &default_prediction_method;
                                        return std::make_unique<RadioButtonType>(radiobutton_name, original_window, default_checked, UpdateImages, nullptr, *this, prediction_method); //Only process checking, not unchecking (no callback and thus no update)
                                      });
    }

    static constexpr auto original_window_name = "Original";
    static constexpr auto transformed_window_name = "Original and its DCT";
    static constexpr auto predicted_window_name = "Predicted";
    static constexpr auto predicted_transformed_window_name = "Residual and its DCT";
    static constexpr auto prediction_window_name = "Prediction illustration";
  public:  
    prediction_data(const cv::Mat &image)
     : original_window(original_window_name),
       transformed_window(transformed_window_name),
       predicted_window(predicted_window_name),
       predicted_transformed_window(predicted_transformed_window_name),
       prediction_window(prediction_window_name),
       original_and_transformed_windows({&original_window, &transformed_window}, imgutils::WindowAlignment::Vertical),
       predicted_and_transformed_windows({&predicted_window, &predicted_transformed_window}, imgutils::WindowAlignment::Vertical),
       all_windows({&original_and_transformed_windows, &predicted_and_transformed_windows, &prediction_window}, imgutils::WindowAlignment::Horizontal),
       region(GetCenterRegion(image))
    {
      AddRadioButtons();
      transformed_window.SetAlwaysShowEnhanced(); //This window needs to be enhanced to show overlays
      predicted_transformed_window.SetAlwaysShowEnhanced(); //This window needs to be enhanced to show overlays
      predicted_window.SetPositionLikeEnhanced(); //Position this window aligned with the original (enhanced) one
      prediction_window.SetPositionLikeEnhanced(); //Position this window aligned with the original (enhanced) one
      UpdateImages(*this, default_prediction_method); //Update with default values
    }
    
    void ShowImages()
    {
      all_windows.ShowInteractive([this]()
                                        {
                                          UpdateImages(*this, default_prediction_method); //Update again so that the status bar entries become visible
                                        });
    }
};

static void ShowImages(const cv::Mat &image)
{
  prediction_data data(image);
  data.ShowImages();
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
  if (static_cast<unsigned int>(std::min(image.rows, image.cols)) < prediction_data::region_size)
  {
	  std::cerr << "The input image must be at least " << prediction_data::region_size << "x" << prediction_data::region_size << " pixels in size" << std::endl;
    return 3;
  }
  ShowImages(image);
  return 0;
}
