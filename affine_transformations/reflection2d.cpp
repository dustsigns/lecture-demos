//Illustration of 2-D reflection across a line
// Andreas Unterweger, 2021-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "math.hpp"
#include "conf_viz.hpp"

static constexpr auto original_object_name = "Original object";
static constexpr auto transformed_object_name = "Transformed object";
static constexpr auto letter_size = 0.1;
static constexpr auto coordinate_system_size = 4 * letter_size;

static constexpr auto reflection_line_name = "Reflection line";

static void AddCoordinateSystem(vizutils::ConfigurableVisualization &visualization)
{
  cv::viz::WCoordinateSystem coordinate_system(coordinate_system_size);
  visualization.objects.insert(std::make_pair("Coordinate system", coordinate_system));
}

static void AddLetters(vizutils::ConfigurableVisualization &visualization)
{
  constexpr auto text = "A";
  const cv::Point3d letter_position(0, letter_size, 0);
  cv::viz::WText3D original_object(text, letter_position, letter_size, false);
  original_object.setRenderingProperty(cv::viz::OPACITY, 0.5);
  cv::viz::WText3D transformed_object(text, letter_position, letter_size, false); //The actual position is set later
  visualization.objects.insert(std::make_pair(original_object_name, original_object));
  visualization.objects.insert(std::make_pair(transformed_object_name, transformed_object));
}
static void AddReflectionLine(vizutils::ConfigurableVisualization &visualization)
{
  const cv::Point3d start(-coordinate_system_size, 0, 0);
  const cv::Point3d end(coordinate_system_size, 0, 0);
  cv::viz::WLine reflection_line(start, end);
  visualization.objects.insert(std::make_pair(reflection_line_name, reflection_line));
}

static void AddObjects(vizutils::ConfigurableVisualization &visualization)
{ 
  AddCoordinateSystem(visualization);
  AddReflectionLine(visualization);
  AddLetters(visualization);
}

static constexpr auto angle_trackbar_name = "Reflection line angle [°]";

static void ApplyTransformations(vizutils::ConfigurableVisualization &visualization)
{
  const auto angle_degrees = visualization.GetTrackbarValue(angle_trackbar_name);
  const auto angle_radians = -comutils::DegreesToRadians(angle_degrees);
  const cv::Affine3d line_transformation(cv::Vec3d(0, 0, angle_radians)); //Rotation around z axis
  auto &reflection_line = visualization.objects[reflection_line_name];
  reflection_line.setPose(line_transformation);
  const auto original_pose = visualization.objects[original_object_name].getPose();
  const cv::Affine3d::Mat3 reflection_matrix(cos(2 * angle_radians), sin(2 * angle_radians), 0.0, sin(2 * angle_radians), -cos(2 * angle_radians), 0.0, 0.0, 0.0, 1.0); //Reflection around angled line
  const cv::Affine3d transformation = original_pose.concatenate(reflection_matrix);
  auto &transformed_object = visualization.objects[transformed_object_name];
  transformed_object.setPose(transformation);
}

static void AddControls(vizutils::ConfigurableVisualization &visualization)
{
  visualization.AddTrackbar(angle_trackbar_name, ApplyTransformations, 180);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates reflection in two dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  auto visualization_window_name = "2-D reflection across a line";
  auto control_window_name = "2-D reflection parameters";
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
                                         ApplyTransformations(visualization);
                                       });
  return 0;
}
