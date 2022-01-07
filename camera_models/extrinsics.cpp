//Illustration of extrinsic camera parameters
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "math.hpp"
#include "conf_viz.hpp"

static constexpr auto parameter_accuracy = 0.01;

static constexpr auto cone_length = 1.0;

static constexpr char axes[] = {'x', 'y', 'z'};

static auto initializing = true; //Prevents updating incomplete values during initialization

static void AddObjects(vizutils::ConfigurableVisualization &visualization, const char * const model_filename)
{
  if (model_filename)
  {
    const auto mesh = cv::viz::Mesh::load(model_filename);
    cv::viz::WMesh original_mesh(mesh);
    visualization.objects.insert(std::make_pair("Original object", original_mesh));
  }
  else
  {
    constexpr auto cone_radius = 0.5;
    constexpr auto cone_resolution = 100;  
    cv::viz::WCone original_cone(cone_length, cone_radius, cone_resolution);
    visualization.objects.insert(std::make_pair("Original object", original_cone));
  }
}

static cv::Affine3d RoundCameraPose(const cv::Affine3d &old_pose)
{
  cv::Vec3d rotation(old_pose.rvec());
  for (size_t i = 0; i < 3; i++)
    rotation[i] = floor(rotation[i] / parameter_accuracy) * parameter_accuracy;
  cv::Vec3d translation(old_pose.translation());
  for (size_t i = 0; i < 3; i++)
    translation[i] = floor(translation[i] / parameter_accuracy) * parameter_accuracy;
  const cv::Affine3d pose(rotation, translation);
  return pose;
}

static std::string GetRotationTrackbarName(const char axis)
{
  using namespace std::string_literals;
  const auto trackbar_name = "Rotation ("s + axis + ") [°]";
  return trackbar_name;
}

static std::string GetTranslationTrackbarName(const char axis)
{
  using namespace std::string_literals;
  const auto trackbar_name = "Translation ("s + axis + ") [px]";
  return trackbar_name;
}

static void UpdateCameraPose(vizutils::ConfigurableVisualization &visualization)
{
  if (initializing) //Don't update during initialization
    return;

  double rotation_angles[comutils::arraysize(axes)], translation_offsets[comutils::arraysize(axes)];
  for (size_t i = 0; i < comutils::arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetRotationTrackbarName(axis);
    const auto rotation_degrees = visualization.GetTrackbarValue(trackbar_name);
    rotation_angles[i] = comutils::DegreesToRadians(rotation_degrees);
  }
  for (size_t i = 0; i < comutils::arraysize(axes); i++)
  { 
    const auto &axis = axes[i];   
    const auto trackbar_name = GetTranslationTrackbarName(axis);
    const auto translation_unscaled = visualization.GetTrackbarValue(trackbar_name);
    translation_offsets[i] = translation_unscaled * parameter_accuracy;
  }
  const cv::Vec3d rotation(rotation_angles);
  const cv::Vec3d translation(translation_offsets);
  const cv::Affine3d pose(rotation, translation);
  const cv::Affine3d rounded_pose = RoundCameraPose(pose);
  visualization.SetViewerPose(rounded_pose);
}

static void AddControls(vizutils::ConfigurableVisualization &visualization)
{
  for (const auto &axis : axes)
  {
    const auto trackbar_name = GetRotationTrackbarName(axis);
    visualization.AddTrackbar(trackbar_name, UpdateCameraPose, 360);
  }
  for (size_t i = 0; i < comutils::arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetTranslationTrackbarName(axis);
    const auto limit = i == comutils::arraysize(axes) - 1 ? 500: 50;
    visualization.AddTrackbar(trackbar_name, UpdateCameraPose, limit, -limit);
  }
}

static void InitializeControlValues(vizutils::ConfigurableVisualization &visualization, const cv::Affine3d &pose)
{
  const auto rotation = pose.rvec();
  for (size_t i = 0; i < comutils::arraysize(axes); i++)
  {
    const auto &axis = axes[i];
    const auto trackbar_name = GetRotationTrackbarName(axis);
    const auto rotation_radians = comutils::RadiansToDegrees(rotation[i]);
    visualization.UpdateTrackbarValue(trackbar_name, rotation_radians);
  }
  const auto translation = pose.translation();
  for (size_t i = 0; i < comutils::arraysize(axes); i++)
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
    std::cout << "Illustrates the effect of the extrinsic parameters of a pinhole camera." << std::endl;
    std::cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << std::endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
  constexpr auto visualization_window_name = "Camera view";
  constexpr auto control_window_name = "Extrinsic camera parameters";
  vizutils::ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization, model_filename);
  AddControls(visualization);
  visualization.ShowWindows([&visualization](const cv::Affine3d &pose)
                                            {
                                              const cv::Affine3d rounded_pose = RoundCameraPose(pose);
                                              InitializeControlValues(visualization, rounded_pose);
                                              return rounded_pose;
                                            });
  return 0;
}
