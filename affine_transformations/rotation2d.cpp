//Illustration of 2-D rotation around the origin
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "math.hpp"
#include "conf_viz.hpp"

static constexpr auto transformed_object_name = "Transformed object";
static constexpr auto letter_size = 0.1;

static constexpr auto center_line_name = "Center line";

static void AddCoordinateSystem(vizutils::ConfigurableVisualization &visualization)
{
  cv::viz::WCoordinateSystem coordinate_system(4 * letter_size);
  //TODO: Hide z axis or rotate it away so that it does not hide the center line
  visualization.objects.insert(std::make_pair("Coordinate system", coordinate_system));
}

static cv::Point3d AddLetters(vizutils::ConfigurableVisualization &visualization)
{
  constexpr auto text = "A";
  const cv::Point3d letter_position(0, letter_size, 0);
  cv::viz::WText3D original_object(text, letter_position, letter_size, false);
  original_object.setRenderingProperty(cv::viz::OPACITY, 0.5);
  cv::viz::WText3D transformed_object(text, letter_position, letter_size, false);
  visualization.objects.insert(std::make_pair("Original object", original_object));
  visualization.objects.insert(std::make_pair(transformed_object_name, transformed_object));
  return letter_position + cv::Point3d(letter_size / 3, 0, 0); //Connect line to point within letter
}

static void AddCenterLine(vizutils::ConfigurableVisualization &visualization, const cv::Point3d &letter_position)
{
  const cv::Point3d origin(0, 0, 0);
  cv::viz::WArrow center_line(origin, letter_position);
  visualization.objects.insert(std::make_pair(center_line_name, center_line));
}

static void AddObjects(vizutils::ConfigurableVisualization &visualization)
{
  AddCoordinateSystem(visualization);
  const auto letter_position = AddLetters(visualization);
  AddCenterLine(visualization, letter_position);
}

static constexpr auto angle_trackbar_name = "Angle [°]";

static void ApplyTransformations(vizutils::ConfigurableVisualization &visualization)
{
  const auto angle_degrees = visualization.GetTrackbarValue(angle_trackbar_name);
  const cv::Affine3d transformation(cv::Vec3d(0, 0, comutils::DegreesToRadians(angle_degrees))); //Rotation around z axis
  auto &transformed_object = visualization.objects[transformed_object_name];
  transformed_object.setPose(transformation);
  auto &transformed_center_line = visualization.objects[center_line_name];
  transformed_center_line.setPose(transformation);
}

static void AddControls(vizutils::ConfigurableVisualization &visualization)
{
  visualization.AddTrackbar(angle_trackbar_name, ApplyTransformations, 360);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates rotation in two dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  constexpr auto visualization_window_name = "2-D rotation around origin";
  constexpr auto control_window_name = "2-D rotation parameters";
  vizutils::ConfigurableVisualization visualization(visualization_window_name, control_window_name);
  AddObjects(visualization);
  AddControls(visualization);
  visualization.ShowWindows(nullptr, [](vizutils::ConfigurableVisualization &visualization)
                                       {
                                         const auto old_camera = visualization.GetCamera();
                                         const auto focal_length = old_camera.getFocalLength();
                                         cv::viz::Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                         camera.setClip(cv::Vec2d(-0.01, 0)); //Only show small portion of space (effectively hides the z axis)
                                         visualization.SetCamera(camera);
                                       });
  return 0;
}
