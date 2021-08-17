//Illustration of extrinsic camera parameters
// Andreas Unterweger, 2017-2021
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "math.hpp"
#include "conf_viz.hpp"

using namespace std;

using namespace cv;
using namespace viz;

using namespace comutils;
using namespace vizutils;

static constexpr auto parameter_accuracy = 0.01;

static constexpr auto cone_length = 1.0;

static constexpr char axes[] = {'x', 'y', 'z'};

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

static string GetRotationTrackbarName(const char axis)
{
  const auto trackbar_name = "Rotation ("s + axis + ") [°]";
  return trackbar_name;
}

static string GetTranslationTrackbarName(const char axis)
{
  const auto trackbar_name = "Translation ("s + axis + ") [px]";
  return trackbar_name;
}

static void UpdateCameraPose(ConfigurableVisualization &visualization)
{
  if (initializing) //Don't update during initialization
    return;

  double rotation_angles[arraysize(axes)], translation_offsets[arraysize(axes)];
  for (size_t i = 0; i < arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetRotationTrackbarName(axis);
    const auto rotation_degrees = visualization.GetTrackbarValue(trackbar_name);
    rotation_angles[i] = DegreesToRadians(rotation_degrees);
  }
  for (size_t i = 0; i < arraysize(axes); i++)
  { 
    const auto &axis = axes[i];   
    const auto trackbar_name = GetTranslationTrackbarName(axis);
    const auto translation_unscaled = visualization.GetTrackbarValue(trackbar_name);
    translation_offsets[i] = translation_unscaled * parameter_accuracy;
  }
  const Vec3d rotation(rotation_angles);
  const Vec3d translation(translation_offsets);
  const Affine3d pose(rotation, translation);
  const Affine3d rounded_pose = RoundCameraPose(pose);
  visualization.SetViewerPose(rounded_pose);
}

static void AddControls(ConfigurableVisualization &visualization)
{
  for (const auto &axis : axes)
  {
    const auto trackbar_name = GetRotationTrackbarName(axis);
    visualization.AddTrackbar(trackbar_name, UpdateCameraPose, 360);
  }
  for (size_t i = 0; i < arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetTranslationTrackbarName(axis);
    const auto limit = i == arraysize(axes) - 1 ? 500: 50;
    visualization.AddTrackbar(trackbar_name, UpdateCameraPose, limit, -limit);
  }
}

static void InitializeControlValues(ConfigurableVisualization &visualization, const Affine3d &pose)
{
  const auto rotation = pose.rvec();
  for (size_t i = 0; i < arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetRotationTrackbarName(axis);
    const auto rotation_radians = RadiansToDegrees(rotation[i]);
    visualization.UpdateTrackbarValue(trackbar_name, rotation_radians);
  }
  const auto translation = pose.translation();
  for (size_t i = 0; i < arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetTranslationTrackbarName(axis);
    const auto translation_scaled = translation[i] / parameter_accuracy;
    visualization.UpdateTrackbarValue(trackbar_name, translation_scaled);
  }
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
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
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
