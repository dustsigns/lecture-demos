//Illustration of 3-D reflection across a plane
// Andreas Unterweger, 2023
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <algorithm>
#include <cmath>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "confviz.hpp"

class reflection_data
{
  protected:
    static constexpr char axes[] = {'X', 'Y', 'Z'};
    static constexpr auto &default_axis = axes[0]; //X by default
    
    vizutils::ConfigurableVisualizationWindow configurable_visualization;
    
    using RadioButtonType = imgutils::RadioButton<reflection_data&, const char>;
    std::unique_ptr<RadioButtonType> reflection_radiobuttons[comutils::arraysize(axes)];
      
    std::unique_ptr<cv::viz::Widget3D> coordinate_system;
    std::unique_ptr<cv::viz::Widget3D> original_object;
    std::unique_ptr<cv::viz::Widget3D> transformed_object;
    std::unique_ptr<cv::viz::Widget3D> reflection_plane;
    
    static constexpr auto cone_length = 0.2;
    static constexpr auto cone_radius = cone_length / 2;
    
    void AddCoordinateSystem()
    {
      coordinate_system = std::make_unique<cv::viz::WCoordinateSystem>(cone_radius); //TODO: Use adaptive scaling
      configurable_visualization.visualization_window.AddWidget("Coordinate system", coordinate_system.get());
    }
    
    void AddObjects(const char * const model_filename)
    {
      if (model_filename)
      {
        const auto mesh = cv::viz::Mesh::load(model_filename);
        original_object = std::make_unique<cv::viz::WMesh>(mesh);
        transformed_object = std::make_unique<cv::viz::WMesh>(mesh);
      }
      else
      {
        constexpr auto cone_resolution = 100;
        original_object = std::make_unique<cv::viz::WCone>(cone_length, cone_radius, cone_resolution);
        transformed_object = std::make_unique<cv::viz::WCone>(cone_length, cone_radius, cone_resolution);
      }
      original_object->setRenderingProperty(cv::viz::OPACITY, 0.5);
      configurable_visualization.visualization_window.AddWidget("Original object", original_object.get());
      configurable_visualization.visualization_window.AddWidget("Transformed object", transformed_object.get());
     
      reflection_plane = std::make_unique<cv::viz::WPlane>(cv::Size2d(2 * cone_length, 2 * cone_length), cv::viz::Color::yellow());
      reflection_plane->setRenderingProperty(cv::viz::OPACITY, 0.5);
      configurable_visualization.visualization_window.AddWidget("Reflection plane", reflection_plane.get());
    }
    
    static void UpdateImage(reflection_data &data, const char axis)
    {
      const auto original_pose = data.original_object->getPose();
      auto reflection_matrix = cv::Affine3d::Mat3::eye();
      assert(axis >= 'X' && axis <= 'Z');
      const auto axis_index = axis - 'X';
      reflection_matrix(axis_index, axis_index) = -1;
      const cv::Affine3d transformation = original_pose.concatenate(cv::Affine3d(reflection_matrix));
      data.transformed_object->setPose(transformation);
      
      cv::Vec3d plane_vector; //Plane is already in correct position for Z axis (XY plane) rotation
      if (axis == 'X')
        plane_vector['Y' - 'X'] = M_PI / 2; //X axis (YZ plane) requires 90° Y rotation
      else if (axis == 'Y')
        plane_vector['X' - 'X'] = M_PI / 2; //Y axis (XZ plane) requires 90° X rotation
      const auto plane_matrix = cv::Affine3d(plane_vector);
      data.reflection_plane->setPose(plane_matrix);
    }
    
    void AddControls()
    {
      std::transform(std::begin(axes), std::end(axes), std::begin(reflection_radiobuttons),
                     [this](const auto &axis)
                           {
                             auto plane_name = std::string(axes); //Start with all axes
                             plane_name.erase(std::remove(plane_name.begin(), plane_name.end(), axis), plane_name.end()); //Remove axis from all axes to get plane
                             const auto radiobutton_name = plane_name + " plane";
                             const auto default_checked = &axis == &default_axis;
                             return std::make_unique<RadioButtonType>(radiobutton_name, configurable_visualization.configuration_window, default_checked, UpdateImage, nullptr, *this, axis);
                           });
    }
    
    static constexpr auto visualization_window_name = "3-D reflection across a plane";
    static constexpr auto control_window_name = "3-D reflection parameters";
  public:
    reflection_data(const char * const model_filename)
     : configurable_visualization(visualization_window_name, control_window_name)
    {
      AddCoordinateSystem();
      AddObjects(model_filename);
      AddControls();
    }
    
    void ShowImage()
    {
      configurable_visualization.ShowInteractive([this]()
                                                       {
                                                         auto &window = configurable_visualization.visualization_window;
                                                         const auto old_camera = window.GetCamera();
                                                         const auto focal_length = old_camera.getFocalLength() / 2; //Reduce focal length so that object is not clipped
                                                         const cv::viz::Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                                         window.SetCamera(camera);
                                                         auto window_pose = window.GetViewerPose();
                                                         window_pose = window_pose.rotate(cv::Vec3d(0.1, 0.1, 0)); //Rotate slightly around X and Y axis to make X and Y axis reflection planes visible
                                                         window.SetViewerPose(window_pose);
                                                         UpdateImage(*this, default_axis); //Update default (transformed) positions
                                                       });
    }
};

static void ShowImage(const char * const model_filename)
{
  reflection_data data(model_filename);
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    std::cout << "Illustrates reflection in three dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << std::endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
  ShowImage(model_filename);
  return 0;
}
