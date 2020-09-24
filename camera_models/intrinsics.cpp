//Illustration of intrinsic camera parameters
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/viz.hpp>

#include "conf_viz.hpp"

using namespace std;

using namespace cv;
using namespace viz;

using namespace vizutils;

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

static constexpr auto focal_length_x_trackbar_name = "Focal length (x) [px]";
static constexpr auto focal_length_y_trackbar_name = "Focal length (y) [px]";
static constexpr auto principal_point_x_trackbar_name = "Image center (x) [px]";
static constexpr auto principal_point_y_trackbar_name = "Image center (y) [px]";

static void UpdateCamera(ConfigurableVisualization &visualization)
{
  if (initializing) //Don't update during initialization
    return;
    
  const auto focal_length_x = visualization.GetTrackbarValue(focal_length_x_trackbar_name);
  const auto focal_length_y = visualization.GetTrackbarValue(focal_length_y_trackbar_name);
  const auto principal_point_x = visualization.GetTrackbarValue(principal_point_x_trackbar_name);
  const auto principal_point_y = visualization.GetTrackbarValue(principal_point_y_trackbar_name);
  const auto old_camera = visualization.GetCamera();
  Camera camera(focal_length_x, focal_length_y, principal_point_x, principal_point_y, old_camera.getWindowSize());
  visualization.SetCamera(camera); //TODO: This toggles every second time on identical input values and eventually drifts away completely
}

static void AddControls(ConfigurableVisualization &visualization)
{
  visualization.AddTrackbar(focal_length_x_trackbar_name, UpdateCamera, 2 * ConfigurableVisualization::window_width); //TODO: Find a more meaningful maximum value
  visualization.AddTrackbar(focal_length_y_trackbar_name, UpdateCamera, 2 * ConfigurableVisualization::window_height); //TODO: Find a more meaningful maximum value
  visualization.AddTrackbar(principal_point_x_trackbar_name, UpdateCamera, ConfigurableVisualization::window_width);
  visualization.AddTrackbar(principal_point_y_trackbar_name, UpdateCamera, ConfigurableVisualization::window_height);
}

static void InitializeControlValues(ConfigurableVisualization &visualization)
{
  const auto camera = visualization.GetCamera();
  const auto focal_length = camera.getFocalLength();
  const auto principal_point = camera.getPrincipalPoint();
  const auto focal_length_x = static_cast<int>(focal_length[0]);
  const auto focal_length_y = static_cast<int>(focal_length[1]);
  visualization.UpdateTrackbarValue(focal_length_x_trackbar_name, focal_length_x);
  visualization.UpdateTrackbarValue(focal_length_y_trackbar_name, focal_length_y);
  visualization.UpdateTrackbarValue(principal_point_x_trackbar_name, principal_point[0]);
  visualization.UpdateTrackbarValue(principal_point_y_trackbar_name, principal_point[1]);
  initializing = false; //Done initializing
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    cout << "Illustrates the effect of the intrinsic parameters of a pinhole camera." << endl;
    cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
  constexpr auto visualization_window_name = "Camera view";
  constexpr auto control_window_name = "Intrinsic camera parameters";
  ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization, model_filename);
  AddControls(visualization);
  visualization.ShowWindows([&visualization](const Affine3d &pose)
                                            {
                                              InitializeControlValues(visualization); //Initialize controls here as the camera is only properly initialized at this point
                                              return pose;
                                            });
  return 0;
}
