//Illustration of color space quantization
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <map>

#include <opencv2/viz.hpp>

#include "confviz.hpp"

class quantization_data
{
  protected:
    static auto constexpr maximum_quantization_level = 256;
    static_assert(maximum_quantization_level > 1, "The maximum number of quantization levels must be at least 2");
    static auto constexpr minimum_quantization_level = 2;
    static_assert(maximum_quantization_level > 1, "The minimum number of quantization levels must be at least 2");
    static_assert(maximum_quantization_level >= minimum_quantization_level, "The maximum number of quantization levels must greater than the minimum number of quantization levels");
    static auto constexpr default_quantization_level = 4;
    static_assert(default_quantization_level >= minimum_quantization_level && default_quantization_level <= maximum_quantization_level, "The initial quantization level must not be greater than the maximum number of quantization levels, nor must it be smaller than the minimum number of quantization levels");
    
    vizutils::ConfigurableVisualizationWindow configurable_visualization;
    
    using TrackBarType = imgutils::TrackBar<quantization_data&>;
    TrackBarType quantization_level_trackbar;
    
    std::map<std::string, cv::viz::WCube> rgb_cubes;
    
    void ClearRGBCubes()
    {
      rgb_cubes.clear();
      configurable_visualization.visualization_window.ClearWidgets();
    }
    
    void AddRGBCube(const int r, const int g, const int b, const double element_size, size_t &counter)
    {
      const cv::Point3d start_point(r * element_size, g * element_size, b * element_size);
      const cv::Point3d end_point((r + 1) * element_size, (g + 1) * element_size, (b + 1) * element_size);
      const cv::viz::Color color(b * element_size, g * element_size, r * element_size);
      cv::viz::WCube cube(start_point, end_point, false, color);
      rgb_cubes.emplace(std::to_string(counter++), std::move(cube));
    }
    
    void PrepareRGBCubes()
    {
      const auto number_of_cubes_per_dimension = quantization_level_trackbar.GetValue();
      const auto element_size = static_cast<double>(maximum_quantization_level) / number_of_cubes_per_dimension;
      size_t counter = 0;
      for (const auto coordinate : { 0, number_of_cubes_per_dimension - 1 }) //Only render first and last coordinate/cube as the others are hidden inside and thus invisible
      {
        for (int r = 0; r < number_of_cubes_per_dimension; r++)
        {
          for (int g = 0; g < number_of_cubes_per_dimension; g++)
            AddRGBCube(r, g, coordinate, element_size, counter);
        }
        for (int r = 0; r < number_of_cubes_per_dimension; r++)
        {
          for (int b = 0; b < number_of_cubes_per_dimension; b++)
            AddRGBCube(r, coordinate, b, element_size, counter);
        }
        for (int g = 0; g < number_of_cubes_per_dimension; g++)
        {
          for (int b = 0; b < number_of_cubes_per_dimension; b++)
            AddRGBCube(coordinate, g, b, element_size, counter);
        }
      }
    }
    
    void AddRGBCubes()
    {
      for (auto &[name, cube] : rgb_cubes)
        configurable_visualization.visualization_window.AddWidget(name, &cube);
    }
    
    static void UpdateImage(quantization_data &data)
    {
      data.ClearRGBCubes();
      data.PrepareRGBCubes();
      data.AddRGBCubes();
    }
    
    static constexpr auto visualization_window_name = "Color space elements";
    static constexpr auto control_window_name = "Quantization parameters";
    static constexpr auto quantization_level_trackbar_name = "Elements";
  public:
    quantization_data()
     : configurable_visualization(visualization_window_name, control_window_name),
       quantization_level_trackbar(quantization_level_trackbar_name, configurable_visualization.configuration_window, maximum_quantization_level, minimum_quantization_level, default_quantization_level, UpdateImage, *this)
    {
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImage()
    {
      configurable_visualization.ShowInteractive();
    }
};

static void ShowImage()
{
  quantization_data data;
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates the effect of quantization on the RGB color space." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowImage();
  return 0;
}
