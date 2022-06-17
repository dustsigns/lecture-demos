//Illustration of 2-D rotation around the origin
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "math.hpp"
#include "confviz.hpp"

class rotation_data
{
  protected:
    vizutils::ConfigurableVisualizationWindow configurable_visualization;
      
    using TrackBarType = imgutils::TrackBar<rotation_data&>;
    TrackBarType rotation_trackbar;
    
    std::unique_ptr<cv::viz::Widget3D> coordinate_system;
    std::unique_ptr<cv::viz::Widget3D> original_object;
    std::unique_ptr<cv::viz::Widget3D> transformed_object;
    std::unique_ptr<cv::viz::Widget3D> transformed_center_line;
    
    static constexpr auto letter_size = 0.1;
    
    void AddCoordinateSystem()
    {
      coordinate_system = std::make_unique<cv::viz::WCoordinateSystem>(4 * letter_size);
      configurable_visualization.visualization_window.AddWidget("Coordinate system", coordinate_system.get());
    }
    
    cv::Point3d AddObjects()
    {
      constexpr auto text = "A";
      const cv::Point3d letter_position(0, letter_size, 0);
      original_object = std::make_unique<cv::viz::WText3D>(text, letter_position, letter_size);
      transformed_object = std::make_unique<cv::viz::WText3D>(text, letter_position, letter_size);
      original_object->setRenderingProperty(cv::viz::OPACITY, 0.5);
      configurable_visualization.visualization_window.AddWidget("Original object", original_object.get());
      configurable_visualization.visualization_window.AddWidget("Transformed object", transformed_object.get());
      return letter_position;
    }
    
    void AddHelperLine(const cv::Point3d &letter_position)
    { 
      const cv::Point3d origin(0, 0, 0);
      const auto connected_letter_position = letter_position + cv::Point3d(letter_size / 3, 0, 0); //Connect line to point within letter
      transformed_center_line = std::make_unique<cv::viz::WArrow>(origin, connected_letter_position);
      configurable_visualization.visualization_window.AddWidget("Center line", transformed_center_line.get());
    }
    
    static void UpdateImage(rotation_data &data)
    {
      const auto angle_degrees = data.rotation_trackbar.GetValue();
      const auto angle = comutils::DegreesToRadians(angle_degrees);
      const cv::Affine3d transformation(cv::Vec3d(0, 0, angle)); //Rotation around z axis
      data.transformed_object->setPose(transformation);
      data.transformed_center_line->setPose(transformation);
    }
    
    static constexpr auto visualization_window_name = "2-D rotation around the origin";
    static constexpr auto control_window_name = "2-D rotation parameters";
    static constexpr auto trackbar_name = "Angle [°]";
  public:
    rotation_data()
     : configurable_visualization(visualization_window_name, control_window_name),
       rotation_trackbar(trackbar_name, configurable_visualization.configuration_window, 360, 0, 0, UpdateImage, *this)
    {
      AddCoordinateSystem();
      const auto letter_position = AddObjects();
      AddHelperLine(letter_position);
    }
    
    void ShowImage()
    {
      configurable_visualization.ShowInteractive([this]()
                                                       {
                                                         auto &window = configurable_visualization.visualization_window;
                                                         const auto old_camera = window.GetCamera();
                                                         const auto focal_length = old_camera.getFocalLength() / 2; //Reduce focal length so that object is not clipped
                                                         cv::viz::Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                                         camera.setClip(cv::Vec2d(-0.01, 0)); //Only show small portion of space (effectively hides the z axis)
                                                         window.SetCamera(camera);
                                                       });
    }
};

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates rotation in two dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  rotation_data data;
  data.ShowImage();
  return 0;
}
