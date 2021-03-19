//Illustration of 2-D reflection across a line
// Andreas Unterweger, 2021
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

static constexpr auto original_object_name = "Original object";
static constexpr auto transformed_object_name = "Transformed object";
static constexpr auto letter_size = 0.1;
static constexpr auto coordinate_system_size = 4 * letter_size;

static constexpr auto reflection_line_name = "Reflection line";

static void AddCoordinateSystem(ConfigurableVisualization &visualization)
{
  WCoordinateSystem coordinate_system(coordinate_system_size);
  visualization.objects.insert(make_pair("Coordinate system", coordinate_system));
}

static void AddLetters(ConfigurableVisualization &visualization)
{
  constexpr auto text = "A";
  const Point3d letter_position(0, letter_size, 0);
  WText3D original_object(text, letter_position, letter_size, false, Color::gray());
  original_object.setRenderingProperty(OPACITY, 0.5);
  WText3D transformed_object(text, letter_position, letter_size, false); //The actual position is set later
  visualization.objects.insert(make_pair(original_object_name, original_object));
  visualization.objects.insert(make_pair(transformed_object_name, transformed_object));
}
static void AddReflectionLine(ConfigurableVisualization &visualization)
{
  const Point3d start(-coordinate_system_size, 0, 0);
  const Point3d end(coordinate_system_size, 0, 0);
  WLine reflection_line(start, end);
  visualization.objects.insert(make_pair(reflection_line_name, reflection_line));
}

static void AddObjects(ConfigurableVisualization &visualization)
{ 
  AddCoordinateSystem(visualization);
  AddReflectionLine(visualization);
  AddLetters(visualization);
}

static constexpr auto angle_trackbar_name = "Reflection line angle [°]";

static void ApplyTransformations(ConfigurableVisualization &visualization)
{
  const auto angle_degrees = visualization.GetTrackbarValue(angle_trackbar_name);
  const auto angle_radians = -DegreesToRadians(angle_degrees);
  const Affine3d line_transformation(Vec3d(0, 0, angle_radians)); //Rotation around z axis
  auto &reflection_line = visualization.objects[reflection_line_name];
  reflection_line.setPose(line_transformation);
  const auto original_pose = visualization.objects[original_object_name].getPose();
  const Affine3d::Mat3 reflection_matrix(cos(2 * angle_radians), sin(2 * angle_radians), 0.0, sin(2 * angle_radians), -cos(2 * angle_radians), 0.0, 0.0, 0.0, 1.0); //Reflection around angled line
  const Affine3d transformation = original_pose.concatenate(reflection_matrix);
  auto &transformed_object = visualization.objects[transformed_object_name];
  transformed_object.setPose(transformation);
}

static void AddControls(ConfigurableVisualization &visualization)
{
  visualization.AddTrackbar(angle_trackbar_name, ApplyTransformations, 180);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates reflection in two dimensions." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  auto visualization_window_name = "2-D reflection across a line";
  auto control_window_name = "2-D reflection parameters";
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
                                              ApplyTransformations(visualization); //Refresh position of transformed letter
                                              return pose;
                                            });
  return 0;
}
