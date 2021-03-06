//Illustration of 2-D scaling
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "conf_viz.hpp"

using namespace std;

using namespace cv;
using namespace viz;

using namespace vizutils;

static constexpr auto transformed_object_name = "Transformed object";
static constexpr auto letter_size = 0.1;

static void AddCoordinateSystem(ConfigurableVisualization &visualization)
{
  WCoordinateSystem coordinate_system(4 * letter_size);
  visualization.objects.insert(make_pair("Coordinate system", coordinate_system));
}

static void Add2DObjects(ConfigurableVisualization &visualization)
{
  constexpr auto text = "A";
  WText3D original_object(text, Point3d(0, letter_size, 0), letter_size, false, Color::gray());
  original_object.setRenderingProperty(OPACITY, 0.5);
  WText3D transformed_object(text, Point3d(0, letter_size, 0), letter_size, false);
  visualization.objects.insert(make_pair("Original object", original_object));
  visualization.objects.insert(make_pair(transformed_object_name, transformed_object));
}

static void AddObjects(ConfigurableVisualization &visualization)
{ 
  AddCoordinateSystem(visualization);
  Add2DObjects(visualization);
}

static constexpr auto x_zoom_trackbar_name = "X zoom [%]";
static constexpr auto y_zoom_trackbar_name = "Y zoom [%]";

static constexpr decltype(x_zoom_trackbar_name) trackbar_names[] { x_zoom_trackbar_name, y_zoom_trackbar_name };

static void ApplyTransformations(ConfigurableVisualization &visualization)
{
  const auto x_zoom_percent = visualization.GetTrackbarValue(x_zoom_trackbar_name);
  const auto y_zoom_percent = visualization.GetTrackbarValue(y_zoom_trackbar_name);
  const Vec3d zoom(x_zoom_percent / 100.0, y_zoom_percent / 100.0, 1.0);
  const Affine3d transformation = Affine3d(Affine3d::Mat3::diag(zoom));
  auto &transformed_object = visualization.objects[transformed_object_name];
  transformed_object.setPose(transformation);
}

static void AddControls(ConfigurableVisualization &visualization)
{
  for (const auto &trackbar_name : trackbar_names)
    visualization.AddTrackbar(trackbar_name, ApplyTransformations, 200, 0, 100);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates scaling in two dimensions." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  constexpr auto visualization_window_name = "2-D scaling";
  constexpr auto control_window_name = "2-D scaling parameters";
  ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization);
  AddControls(visualization);
  visualization.ShowWindows([&visualization](const Affine3d &pose)
                                            {
                                              const auto old_camera = visualization.GetCamera();
                                              const auto focal_length = old_camera.getFocalLength() / 2; //Reduce focal length so that object is not clipped
                                              Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                              camera.setClip(Vec2d(-0.01, 0)); //Only show small portion of space (effectively hides the z axis)
                                              visualization.SetCamera(camera);
                                              return pose;
                                            });
  return 0;
}
