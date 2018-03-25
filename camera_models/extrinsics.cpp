//Illustration of extrinsic camera parameters
// Andreas Unterweger, 2017-2018
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

static constexpr auto parameter_accuracy = 0.01;

static constexpr auto cone_length = 1.0;

static auto initializing = true; //Prevents updating incomplete values during initialization

static void AddObjects(ConfigurableVisualization &visualization, const char * const model_filename)
{
  if (model_filename)
  {
    Mesh mesh = Mesh::load(model_filename);
    WMesh original_mesh(mesh);
    visualization.objects.insert(make_pair("Original object", original_mesh));
  }
  else
  {
    constexpr auto cone_radius = 0.5;
    constexpr auto cone_resolution = 100;  
    WCone original_cone(cone_length, cone_radius, cone_resolution);
    visualization.objects.insert(make_pair("Original object", original_cone));
  }
}

static Affine3d RoundCameraPose(const Affine3d &old_pose)
{
  Vec3d rotation(old_pose.rvec());
  for (size_t i = 0; i < 3; i++)
    rotation[i] = floor(rotation[i] / parameter_accuracy) * parameter_accuracy;
  Vec3d translation(old_pose.translation());
  for (size_t i = 0; i < 3; i++)
    translation[i] = floor(translation[i] / parameter_accuracy) * parameter_accuracy;
  const Affine3d pose(rotation, translation);
  return pose;
}

static constexpr auto rotation_angle_x_trackbar_name = "Rotation (x) [°]";
static constexpr auto rotation_angle_y_trackbar_name = "Rotation (y) [°]";
static constexpr auto rotation_angle_z_trackbar_name = "Rotation (z) [°]";
static constexpr auto translation_x_trackbar_name = "Translation (x) [px]";
static constexpr auto translation_y_trackbar_name = "Translation (y) [px]";
static constexpr auto translation_z_trackbar_name = "Translation (z) [px]";

static void UpdateCameraPose(ConfigurableVisualization &visualization)
{
  if (initializing) //Don't update during initialization
    return;

  const auto translation_x = visualization.GetTrackbarValue(translation_x_trackbar_name);
  const auto translation_y = visualization.GetTrackbarValue(translation_y_trackbar_name);
  const auto translation_z = visualization.GetTrackbarValue(translation_z_trackbar_name);
  const auto rotation_angle_x = visualization.GetTrackbarValue(rotation_angle_x_trackbar_name);
  const auto rotation_angle_y = visualization.GetTrackbarValue(rotation_angle_y_trackbar_name);
  const auto rotation_angle_z = visualization.GetTrackbarValue(rotation_angle_z_trackbar_name); 
  const Vec3d rotation(DegreesToRadians(rotation_angle_x), DegreesToRadians(rotation_angle_y), DegreesToRadians(rotation_angle_z));
  const Vec3d translation(translation_x * parameter_accuracy, translation_y * parameter_accuracy, translation_z * parameter_accuracy);
  const Affine3d pose(rotation, translation);
  const Affine3d rounded_pose = RoundCameraPose(pose);
  visualization.SetViewerPose(rounded_pose);
}

static void AddControls(ConfigurableVisualization &visualization)
{
  visualization.AddTrackbar(rotation_angle_x_trackbar_name, UpdateCameraPose, 360);
  visualization.AddTrackbar(rotation_angle_y_trackbar_name, UpdateCameraPose, 360);
  visualization.AddTrackbar(rotation_angle_z_trackbar_name, UpdateCameraPose, 360);
  visualization.AddTrackbar(translation_x_trackbar_name, UpdateCameraPose, 50, -50);
  visualization.AddTrackbar(translation_y_trackbar_name, UpdateCameraPose, 50, -50);
  visualization.AddTrackbar(translation_z_trackbar_name, UpdateCameraPose, 500, -500);
}

static void InitializeControlValues(ConfigurableVisualization &visualization, const Affine3d &pose)
{
  const auto rotation = pose.rvec();
  visualization.UpdateTrackbarValue(rotation_angle_x_trackbar_name, RadiansToDegrees(rotation[0]));
  visualization.UpdateTrackbarValue(rotation_angle_y_trackbar_name, RadiansToDegrees(rotation[1]));
  visualization.UpdateTrackbarValue(rotation_angle_z_trackbar_name, RadiansToDegrees(rotation[2]));
  const auto translation = pose.translation();
  visualization.UpdateTrackbarValue(translation_x_trackbar_name, translation[0] / parameter_accuracy);
  visualization.UpdateTrackbarValue(translation_y_trackbar_name, translation[1] / parameter_accuracy);
  visualization.UpdateTrackbarValue(translation_z_trackbar_name, translation[2] / parameter_accuracy);
  initializing = false; //Done initializing
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    cout << "Illustrates the effect of the extrinsic parameters of a pinhole camera." << endl;
    cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : NULL;
  constexpr auto visualization_window_name = "Camera view";
  constexpr auto control_window_name = "Extrinsic camera parameters";
  ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization, model_filename);
  AddControls(visualization);
  visualization.ShowWindows([&visualization](const Affine3d &pose)
                                            {
                                              const Affine3d rounded_pose = RoundCameraPose(pose);
                                              InitializeControlValues(visualization, rounded_pose);
                                              return rounded_pose;
                                            });
  return 0;
}
