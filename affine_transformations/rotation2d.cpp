//Illustration of 2-D rotation around the origin
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

static constexpr auto transformed_object_name = "Transformed object";
static constexpr auto letter_size = 0.1;

static constexpr auto center_line_name = "Center line";

static void AddCoordinateSystem(ConfigurableVisualization &visualization)
{
  WCoordinateSystem coordinate_system(4 * letter_size);
  //TODO: Hide z axis or rotate it away so that it does not hide the center line
  visualization.objects.insert(make_pair("Coordinate system", coordinate_system));
}

static Point3d AddLetters(ConfigurableVisualization &visualization)
{
  constexpr auto text = "A";
  const Point3d letter_position(0, letter_size, 0);
  WText3D original_object(text, letter_position, letter_size, false, Color::gray());
  original_object.setRenderingProperty(OPACITY, 0.5);
  WText3D transformed_object(text, letter_position, letter_size, false);
  visualization.objects.insert(make_pair("Original object", original_object));
  visualization.objects.insert(make_pair(transformed_object_name, transformed_object));
  return letter_position + Point3d(letter_size / 3, 0, 0); //Connect line to point within letter
}

static void AddCenterLine(ConfigurableVisualization &visualization, const Point3d &letter_position)
{
  const Point3d origin(0, 0, 0);
  WArrow center_line(origin, letter_position);
  visualization.objects.insert(make_pair(center_line_name, center_line));
}

static void AddObjects(ConfigurableVisualization &visualization)
{
  AddCoordinateSystem(visualization);
  const auto letter_position = AddLetters(visualization);
  AddCenterLine(visualization, letter_position);
}

static constexpr auto angle_trackbar_name = "Angle [°]";

static void ApplyTransformations(ConfigurableVisualization &visualization)
{
  const auto angle_degrees = visualization.GetTrackbarValue(angle_trackbar_name);
  const Affine3d transformation(Vec3d(0, 0, DegreesToRadians(angle_degrees))); //Rotation around z axis
  auto &transformed_object = visualization.objects[transformed_object_name];
  transformed_object.setPose(transformation);
  auto &transformed_center_line = visualization.objects[center_line_name];
  transformed_center_line.setPose(transformation);
}

static void AddControls(ConfigurableVisualization &visualization)
{
  visualization.AddTrackbar(angle_trackbar_name, ApplyTransformations, 360);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates rotation in two dimensions." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  constexpr auto visualization_window_name = "2-D rotation around origin";
  constexpr auto control_window_name = "2-D rotation parameters";
  ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization);
  AddControls(visualization);
  visualization.ShowWindows([&visualization](const Affine3d &pose)
                                            {
                                              const auto old_camera = visualization.GetCamera();
                                              const auto focal_length = old_camera.getFocalLength();
                                              Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                              camera.setClip(Vec2d(-0.01, 0)); //Only show small portion of space (effectively hides the z axis)
                                              visualization.SetCamera(camera);
                                              return pose;
                                            });
  return 0;
}
