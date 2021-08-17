//Illustration of color space quantization
// Andreas Unterweger, 2017-2021
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "conf_viz.hpp"

using namespace std;

using namespace cv;
using namespace viz;

using namespace vizutils;

static auto constexpr default_quantization_levels = 256;

static void RenderRGBCubes(ConfigurableVisualization &visualization, const int number_of_cubes_per_dimension)
{
  assert(number_of_cubes_per_dimension >= 1 && number_of_cubes_per_dimension <= default_quantization_levels);
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
        const auto element_size = static_cast<double>(default_quantization_levels) / number_of_cubes_per_dimension;
        const Point3d start_point(r * element_size, g * element_size, b * element_size);
        const Point3d end_point((r + 1) * element_size, (g + 1) * element_size, (b + 1) * element_size);
        const Color color(b * element_size, g * element_size, r * element_size);
        WCube element(start_point, end_point, false, color);
        visualization.objects.insert(make_pair(to_string(counter++), element));
      }
    }
  }
}

static constexpr auto trackbar_name = "Elements";

static void RenderObjects(ConfigurableVisualization &visualization)
{
  const auto cubes_per_dimension = visualization.GetTrackbarValue(trackbar_name);
  visualization.ClearObjects();
  RenderRGBCubes(visualization, cubes_per_dimension);
  visualization.RedrawObjects();
}

static void AddControls(ConfigurableVisualization &visualization)
{
  visualization.AddTrackbar(trackbar_name, RenderObjects, default_quantization_levels, 2, 4); //Between 2 and 256 quantization levels with 4 being the default
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates the effect of quantization on the RGB color space." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  constexpr auto visualization_window_name = "Color space elements";
  constexpr auto control_window_name = "Quantization parameters";
  ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddControls(visualization);
  visualization.ShowWindows(/*[&visualization](const Affine3d &pose) //TODO: Why does zooming no longer work once the pose is read and set again (even without changes)
                                            {
                                              const Vec3d offset(default_quantization_levels / 2, default_quantization_levels / 2, 4 * default_quantization_levels); //Middle of cube
                                              const auto new_pose = pose.translate(offset); //Move camera so that all possible rotations are visible
                                              return new_pose;
                                            }*/ nullptr,
                            RenderObjects);
  return 0;
}
