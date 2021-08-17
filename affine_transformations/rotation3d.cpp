//Illustration of 3-D rotation around an axis
// Andreas Unterweger, 2017-2021
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "math.hpp"
#include "conf_viz.hpp"

using namespace std;

using namespace cv;
using namespace viz;

using namespace comutils;
using namespace vizutils;

static constexpr auto transformed_object_name = "Transformed object";
static constexpr auto cone_length = 0.2;
static constexpr auto cone_radius = cone_length / 2;

static void AddCoordinateSystem(ConfigurableVisualization &visualization)
{
  WCoordinateSystem coordinate_system(cone_radius); //TODO: Use adaptive scaling
  visualization.objects.insert(make_pair("Coordinate system", coordinate_system));
}

static void Add3DObjects(ConfigurableVisualization &visualization, const char * const model_filename)
{
  if (model_filename)
  {
    Mesh mesh = Mesh::load(model_filename);
    WMesh original_mesh(mesh);
    original_mesh.setRenderingProperty(OPACITY, 0.5);
    WMesh transformed_mesh(mesh);
    visualization.objects.insert(make_pair("Original object", original_mesh));
    visualization.objects.insert(make_pair(transformed_object_name, transformed_mesh));
  }
  else
  {
    constexpr auto cone_resolution = 100;
    WCone original_cone(cone_length, cone_radius, cone_resolution);
    original_cone.setRenderingProperty(OPACITY, 0.5);
    WCone transformed_cone(cone_length, cone_radius, cone_resolution);
    visualization.objects.insert(make_pair("Original object", original_cone));
    visualization.objects.insert(make_pair(transformed_object_name, transformed_cone));
  }
}

static void AddObjects(ConfigurableVisualization &visualization, const char * const model_filename)
{ 
  AddCoordinateSystem(visualization);
  Add3DObjects(visualization, model_filename);
}

static constexpr auto x_angle_trackbar_name = "X angle [°]";
static constexpr auto y_angle_trackbar_name = "Y angle [°]";
static constexpr auto z_angle_trackbar_name = "Z angle [°]";

static constexpr decltype(x_angle_trackbar_name) trackbar_names[] { x_angle_trackbar_name, y_angle_trackbar_name, z_angle_trackbar_name };

static void ApplyTransformations(ConfigurableVisualization &visualization)
{
  const auto x_angle_degrees = visualization.GetTrackbarValue(x_angle_trackbar_name);
  const auto y_angle_degrees = visualization.GetTrackbarValue(y_angle_trackbar_name);
  const auto z_angle_degrees = visualization.GetTrackbarValue(z_angle_trackbar_name);
  Affine3d transformation(Vec3d(DegreesToRadians(x_angle_degrees), 0, 0));
  transformation = transformation.rotate(Vec3d(0, DegreesToRadians(y_angle_degrees), 0));
  transformation = transformation.rotate(Vec3d(0, 0, DegreesToRadians(z_angle_degrees)));
  auto &transformed_object = visualization.objects[transformed_object_name];
  transformed_object.setPose(transformation);
}

static void AddControls(ConfigurableVisualization &visualization)
{
  for (const auto &trackbar_name : trackbar_names)
    visualization.AddTrackbar(trackbar_name, ApplyTransformations, 360);
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    cout << "Illustrates rotation in three dimensions." << endl;
    cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
  const bool use_model = model_filename != nullptr;
  constexpr auto visualization_window_name = "3-D rotation";
  constexpr auto control_window_name = "3-D rotation parameters";
  ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization, model_filename);
  AddControls(visualization);
  if (use_model)
    visualization.ShowWindows([&visualization](const Affine3d &pose)
                                              {
                                                const auto old_camera = visualization.GetCamera();
                                                const auto focal_length = old_camera.getFocalLength() / 2; //Reduce focal length so that object rotation is visible
                                                const Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                                visualization.SetCamera(camera);
                                                return pose;
                                              });
  else
    visualization.ShowWindows([](const Affine3d &pose)
                                {
                                  const Vec3d offset(-cone_length / 2, 0, 0); //Middle of cone
                                  const auto new_pose = pose.translate(offset); //Move camera so that all possible rotations are visible
                                  return new_pose;
                                });
  return 0;
}
