//Illustration of 3-D translation
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "conf_viz.hpp"

static constexpr auto transformed_object_name = "Transformed object";
static constexpr auto cone_length = 0.2;
static constexpr auto cone_radius = cone_length / 2;

static void AddCoordinateSystem(vizutils::ConfigurableVisualization &visualization)
{
  cv::viz::WCoordinateSystem coordinate_system(cone_radius); //TODO: Use adaptive scaling
  visualization.objects.insert(std::make_pair("Coordinate system", coordinate_system));
}

static void Add3DObjects(vizutils::ConfigurableVisualization &visualization, const char * const model_filename)
{
  if (model_filename)
  {
    const auto mesh = cv::viz::Mesh::load(model_filename);
    cv::viz::WMesh original_mesh(mesh);
    original_mesh.setRenderingProperty(cv::viz::OPACITY, 0.5);
    cv::viz::WMesh transformed_mesh(mesh);
    visualization.objects.insert(std::make_pair("Original object", original_mesh));
    visualization.objects.insert(std::make_pair(transformed_object_name, transformed_mesh));
  }
  else
  {
    constexpr auto cone_resolution = 100;
    cv::viz::WCone original_cone(cone_length, cone_radius, cone_resolution);
    original_cone.setRenderingProperty(cv::viz::OPACITY, 0.5);
    cv::viz::WCone transformed_cone(cone_length, cone_radius, cone_resolution);
    visualization.objects.insert(std::make_pair("Original object", original_cone));
    visualization.objects.insert(std::make_pair(transformed_object_name, transformed_cone));
  }
}

static void AddObjects(vizutils::ConfigurableVisualization &visualization, const char * const model_filename)
{ 
  AddCoordinateSystem(visualization);
  Add3DObjects(visualization, model_filename);
}

static constexpr char axes[] = {'X', 'Y', 'Z'};

static std::string GetTrackbarName(const char axis)
{
  using namespace std::string_literals;
  const auto trackbar_name = ""s + axis + " offset";
  return trackbar_name;
}

static void ApplyTransformations(vizutils::ConfigurableVisualization &visualization)
{
  cv::Vec3d offset;
  for (size_t i = 0; i < comutils::arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetTrackbarName(axis);
    const auto offset_percent = visualization.GetTrackbarValue(trackbar_name);
    const auto offset_value = offset_percent / 100.0;
    offset[i] = offset_value;
  }
  const auto transformation = cv::Affine3d::Identity().translate(offset);
  auto &transformed_object = visualization.objects[transformed_object_name];
  transformed_object.setPose(transformation);
}

static void AddControls(vizutils::ConfigurableVisualization &visualization)
{
  for (const auto &axis : axes)
  {
    const auto trackbar_name = GetTrackbarName(axis);
    visualization.AddTrackbar(trackbar_name, ApplyTransformations, 50, -50);
  }
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    std::cout << "Illustrates translation in three dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << std::endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
  constexpr auto visualization_window_name = "3-D translation";
  constexpr auto control_window_name = "3-D translation parameters";
  vizutils::ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization, model_filename);
  AddControls(visualization);
  visualization.ShowWindows(nullptr, [](vizutils::ConfigurableVisualization &visualization)
                                       {
                                         const auto old_camera = visualization.GetCamera();
                                         const auto focal_length = old_camera.getFocalLength() / 2; //Reduce focal length so that object is not clipped
                                         const cv::viz::Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                         visualization.SetCamera(camera);
                                       });
  return 0;
}
