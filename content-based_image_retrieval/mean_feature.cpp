//Illustration of mean RGB feature vectors
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>
#include <map>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/viz.hpp>

#include "vizwin.hpp"

static void AddCoordinateSystem(std::map<std::string, cv::viz::Widget> &widgets)
{
  cv::viz::WCoordinateSystem coordinate_system;
  widgets.emplace("Coordinate system", std::move(coordinate_system));
}

static constexpr auto use_normalization = false; //Set this to true to use additional l1 normalization

static cv::Scalar GetMean(const cv::Mat &image)
{
  assert(image.type() == CV_8UC3);
  const auto mean_value = cv::mean(image);
  if constexpr (use_normalization)
  {
    const auto mean_sum = cv::norm(mean_value, cv::NORM_L1); //Alternative: normalize mean by sum
    return 3 * mean_value / mean_sum; //Normalize mean considering all three components
  }
  else
    return mean_value / 256; //Normalize mean
}

static cv::Mat FixImage(const cv::Mat &image)
{
  cv::Mat fixed_image;
  cv::copyMakeBorder(image, fixed_image, 0, image.rows & 3, 0, image.cols & 3, cv::BORDER_REFLECT_101, cv::Scalar()); //Make sure each image dimension is a multiple of four
  return fixed_image;
}

static void AddImageVisualization(const cv::Mat &image, const std::string &name, std::map<std::string, cv::viz::Widget> &widgets)
{
  constexpr auto thumbnail_size = 0.2;
  
  const auto feature = GetMean(image);
  const auto mean_b = feature[0];
  const auto mean_g = feature[1];
  const auto mean_r = feature[2];
  const cv::Point3d position(mean_r, mean_g, mean_b);
  const cv::viz::Color vector_color(mean_b * 256, mean_g * 256, mean_r * 256);
  cv::viz::WArrow feature_vector(cv::Point3d(), position, thumbnail_size / 100, vector_color);
  widgets.emplace("feature" + name, std::move(feature_vector));
  
  const cv::Mat fixed_image = FixImage(image); //Work around visualization constraints by slightly padding the image
  cv::viz::WImage3D thumbnail(fixed_image, cv::Size2d(thumbnail_size, thumbnail_size), position, position, cv::Vec3d(0, 0, 1));
  widgets.emplace(name, std::move(thumbnail));
}

int main(const int argc, const char * const argv[])
{
  if (argc < 2)
  {
    std::cout << "Illustrates images and their feature vectors when using mean RGB features." << std::endl;
    std::cout << "Usage: " << argv[0] << " <input image 1> [<input image 2> ... [<input image n>]]" << std::endl;
    return 1;
  }
  std::map<std::string, cv::viz::Widget> widgets;
  AddCoordinateSystem(widgets);
  std::vector<const char*> image_filenames(argv + 1, argv + argc); //Start from first actual parameter (ignore program name)
  for (const auto &image_filename : image_filenames)
  {
    const cv::Mat image = cv::imread(image_filename);
    if (image.empty())
    {
      std::cerr << "Could not read input image '" << image_filename << "'" << std::endl;
      return 3;
    }
    AddImageVisualization(image, image_filename, widgets);
  }
  vizutils::VisualizationWindow window("Feature vectors");
  for (auto &[name, widget] : widgets)
    window.AddWidget(name, &widget);
  window.ShowInteractive();
  return 0;
}
