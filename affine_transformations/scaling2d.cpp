//Illustration of 2-D scaling
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "conf_viz.hpp"

static constexpr auto transformed_object_name = "Transformed object";
static constexpr auto letter_size = 0.1;

static void AddCoordinateSystem(vizutils::ConfigurableVisualization &visualization)
{
  cv::viz::WCoordinateSystem coordinate_system(4 * letter_size);
  visualization.objects.insert(std::make_pair("Coordinate system", coordinate_system));
}

static void Add2DObjects(vizutils::ConfigurableVisualization &visualization)
{
  constexpr auto text = "A";
  cv::viz::WText3D original_object(text, cv::Point3d(0, letter_size, 0), letter_size, false);
  original_object.setRenderingProperty(cv::viz::OPACITY, 0.5);
  cv::viz::WText3D transformed_object(text, cv::Point3d(0, letter_size, 0), letter_size, false);
  visualization.objects.insert(std::make_pair("Original object", original_object));
  visualization.objects.insert(std::make_pair(transformed_object_name, transformed_object));
}

static void AddObjects(vizutils::ConfigurableVisualization &visualization)
{ 
  AddCoordinateSystem(visualization);
  Add2DObjects(visualization);
}

static constexpr char axes[] = {'X', 'Y'};

static std::string GetTrackbarName(const char axis)
{
  using namespace std::string_literals;
  const auto trackbar_name = ""s + axis + " zoom [%]";
  return trackbar_name;
}

static void ApplyTransformations(vizutils::ConfigurableVisualization &visualization)
{
  cv::Vec3d zoom;
  for (size_t i = 0; i < comutils::arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetTrackbarName(axis);
    const auto zoom_percent = visualization.GetTrackbarValue(trackbar_name);
    const auto zoom_value = zoom_percent / 100.0;
    zoom[i] = zoom_value;
  }
  auto transformation = cv::Affine3d(cv::Affine3d::Mat3::diag(zoom));
  auto &transformed_object = visualization.objects[transformed_object_name];
  transformed_object.setPose(transformation);
}

static void AddControls(vizutils::ConfigurableVisualization &visualization)
{
  for (const auto &axis : axes)
  {
    const auto trackbar_name = GetTrackbarName(axis);
    visualization.AddTrackbar(trackbar_name, ApplyTransformations, 200, 0, 100);
  }
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates scaling in two dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  constexpr auto visualization_window_name = "2-D scaling";
  constexpr auto control_window_name = "2-D scaling parameters";
  vizutils::ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization);
  AddControls(visualization);
  visualization.ShowWindows(nullptr, [](vizutils::ConfigurableVisualization &visualization)
                                       {
                                         const auto old_camera = visualization.GetCamera();
                                         const auto focal_length = old_camera.getFocalLength() / 2; //Reduce focal length so that object is not clipped
                                         cv::viz::Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                         camera.setClip(cv::Vec2d(-0.01, 0)); //Only show small portion of space (effectively hides the z axis)
                                         visualization.SetCamera(camera);
                                       });
  return 0;
}
