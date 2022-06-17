//Illustration of 2-D reflection across a line
// Andreas Unterweger, 2021-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "math.hpp"
#include "confviz.hpp"

class reflection_data
{
  protected:
    vizutils::ConfigurableVisualizationWindow configurable_visualization;
      
    using TrackBarType = imgutils::TrackBar<reflection_data&>;
    TrackBarType reflection_trackbar;
    
    std::unique_ptr<cv::viz::Widget3D> coordinate_system;
    std::unique_ptr<cv::viz::Widget3D> original_object;
    std::unique_ptr<cv::viz::Widget3D> transformed_object;
    std::unique_ptr<cv::viz::Widget3D> reflection_line;
    
    static constexpr auto letter_size = 0.1;
    static constexpr auto coordinate_system_size = 4 * letter_size;
    
    void AddCoordinateSystem()
    {
      coordinate_system = std::make_unique<cv::viz::WCoordinateSystem>(coordinate_system_size);
      configurable_visualization.visualization_window.AddWidget("Coordinate system", coordinate_system.get());
    }
    
    void AddObjects()
    {
      constexpr auto text = "A";
      const cv::Point3d letter_position(0, letter_size, 0);
      original_object = std::make_unique<cv::viz::WText3D>(text, letter_position, letter_size);
      transformed_object = std::make_unique<cv::viz::WText3D>(text, letter_position, letter_size); //The actual position will be set later
      original_object->setRenderingProperty(cv::viz::OPACITY, 0.5);
      configurable_visualization.visualization_window.AddWidget("Original object", original_object.get());
      configurable_visualization.visualization_window.AddWidget("Transformed object", transformed_object.get());
    }
    
    void AddHelperLine()
    {
      const cv::Point3d start(-coordinate_system_size, 0, 0);
      const cv::Point3d end(coordinate_system_size, 0, 0);
      reflection_line = std::make_unique<cv::viz::WLine>(start, end);
      configurable_visualization.visualization_window.AddWidget("Reflection line", reflection_line.get());
    }
    
    static void UpdateImage(reflection_data &data)
    {
      const auto angle_degrees = data.reflection_trackbar.GetValue();
      const auto angle = comutils::DegreesToRadians(angle_degrees);
      const cv::Affine3d line_transformation(cv::Vec3d(0, 0, angle)); //Rotation around z axis
      data.reflection_line->setPose(line_transformation);
      
      const auto original_pose = data.original_object->getPose();
      const cv::Affine3d::Mat3 reflection_matrix(cos(2 * angle), sin(2 * angle), 0.0, sin(2 * angle), -cos(2 * angle), 0.0, 0.0, 0.0, 1.0); //Reflection around angled line
      const cv::Affine3d transformation = original_pose.concatenate(reflection_matrix);
      data.transformed_object->setPose(transformation);
    }
    
    static constexpr auto visualization_window_name = "2-D reflection across a line";
    static constexpr auto control_window_name = "2-D reflection parameters";
    static constexpr auto trackbar_name = "Reflection line angle [°]";
  public:
    reflection_data()
     : configurable_visualization(visualization_window_name, control_window_name),
       reflection_trackbar(trackbar_name, configurable_visualization.configuration_window, 180, 0, 0, UpdateImage, *this)
    {
      AddCoordinateSystem();
      AddObjects();
      AddHelperLine();
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
                                                         UpdateImage(*this); //Update default (transformed) positions
                                                       });
    }
};

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates reflection in two dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  reflection_data data;
  data.ShowImage();
  return 0;
}
