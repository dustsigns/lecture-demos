//Illustration of color space quantization
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "conf_viz.hpp"

static auto constexpr maximum_quantization_level = 256;

static_assert(maximum_quantization_level > 0, "The number of quantization levels must be at least 1");

static auto constexpr initial_quantization_level = 4;

static_assert(initial_quantization_level <= maximum_quantization_level, "The initial quantization level must not be greater than the number of quantization levels");

static void RenderRGBCubes(vizutils::ConfigurableVisualization &visualization, const int number_of_cubes_per_dimension)
{
  assert(number_of_cubes_per_dimension >= 1 && number_of_cubes_per_dimension <= maximum_quantization_level);
  size_t counter = 0;
  for (int r = 0; r < number_of_cubes_per_dimension; r++)
  {
    for (int g = 0; g < number_of_cubes_per_dimension; g++)
    {
      for (int b = 0; b < number_of_cubes_per_dimension; b++)
      {
        if (r != 0 && r != number_of_cubes_per_dimension - 1 && //Skip cubes on the inside (they are hidden by the outermost cubes)
            g != 0 && g != number_of_cubes_per_dimension - 1 &&
            b != 0 && b != number_of_cubes_per_dimension - 1)
          continue;
        const auto element_size = static_cast<double>(maximum_quantization_level) / number_of_cubes_per_dimension;
        const cv::Point3d start_point(r * element_size, g * element_size, b * element_size);
        const cv::Point3d end_point((r + 1) * element_size, (g + 1) * element_size, (b + 1) * element_size);
        const cv::viz::Color color(b * element_size, g * element_size, r * element_size);
        cv::viz::WCube element(start_point, end_point, false, color);
        visualization.objects.insert(std::make_pair(std::to_string(counter++), element));
      }
    }
  }
}

static constexpr auto trackbar_name = "Elements";

static void RenderObjects(vizutils::ConfigurableVisualization &visualization)
{
  const auto cubes_per_dimension = visualization.GetTrackbarValue(trackbar_name);
  visualization.ClearObjects();
  RenderRGBCubes(visualization, cubes_per_dimension);
  visualization.RedrawObjects();
}

static void AddControls(vizutils::ConfigurableVisualization &visualization)
{
  visualization.AddTrackbar(trackbar_name, RenderObjects, maximum_quantization_level, 2, initial_quantization_level); //Between 2 and 256 quantization levels with 4 being the default
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates the effect of quantization on the RGB color space." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  constexpr auto visualization_window_name = "Color space elements";
  constexpr auto control_window_name = "Quantization parameters";
  vizutils::ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddControls(visualization);
  RenderRGBCubes(visualization, initial_quantization_level);
  visualization.ShowWindows();
  return 0;
}
