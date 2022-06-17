//Illustration of 3-D rotation around an axis
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <algorithm>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "math.hpp"
#include "confviz.hpp"

class rotation_data
{
  protected:
    static constexpr char axes[] = {'X', 'Y', 'Z'};
    
    vizutils::ConfigurableVisualizationWindow configurable_visualization;
      
    using TrackBarType = imgutils::TrackBar<rotation_data&>;
    std::unique_ptr<TrackBarType> rotation_trackbars[comutils::arraysize(axes)];
    
    std::unique_ptr<cv::viz::Widget3D> coordinate_system;
    std::unique_ptr<cv::viz::Widget3D> original_object;
    std::unique_ptr<cv::viz::Widget3D> transformed_object;
    
    static constexpr auto cone_length = 0.2;
    static constexpr auto cone_radius = cone_length / 2;
    
    const bool use_model;
    
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
    }
    
    static void UpdateImage(rotation_data &data)
    {
      cv::Affine3d transformation;
      for (size_t i = 0; i < comutils::arraysize(data.rotation_trackbars); i++)
      {
        const auto &trackbar = data.rotation_trackbars[i];
        const auto angle_degrees = trackbar->GetValue();
        const auto angle = comutils::DegreesToRadians(angle_degrees);
        cv::Vec3d rotation;
        rotation[i] = angle;
        transformation = transformation.rotate(rotation);
      }
      data.transformed_object->setPose(transformation);
    }
    
    void AddControls()
    {
      std::transform(std::begin(axes), std::end(axes), std::begin(rotation_trackbars),
                     [this](const auto axis)
                           {
                             using namespace std::string_literals;
                             const auto trackbar_name = ""s + axis + " angle [°]";
                             return std::make_unique<TrackBarType>(trackbar_name, configurable_visualization.configuration_window, 360, 0, 0, UpdateImage, *this);
                           });
    }
    
    static constexpr auto visualization_window_name = "3-D rotation";
    static constexpr auto control_window_name = "3-D rotation parameters";
  public:
    rotation_data(const char * const model_filename)
     : configurable_visualization(visualization_window_name, control_window_name),
       use_model(model_filename != nullptr)
    {
      AddCoordinateSystem();
      AddObjects(model_filename);
      AddControls();
    }
    
    void ShowImage()
    {
      auto &window = configurable_visualization.visualization_window;
      if (use_model)
      {
        configurable_visualization.ShowInteractive([&window]()
                                                            {
                                                              const auto old_camera = window.GetCamera();
                                                              const auto focal_length = old_camera.getFocalLength() / 2; //Reduce focal length so that object is not clipped
                                                              const cv::viz::Camera camera(focal_length[0], focal_length[1], old_camera.getPrincipalPoint()[0], old_camera.getPrincipalPoint()[1], old_camera.getWindowSize());
                                                              window.SetCamera(camera);
                                                            });
      }
      else
      {
        configurable_visualization.ShowInteractive([&window]()
                                                            {
                                                              const auto pose = window.GetViewerPose();
                                                              const cv::Vec3d offset(-cone_length / 2, 0, 0); //Middle of cone
                                                              const auto new_pose = pose.translate(offset); //Move camera so that all possible rotations are visible
                                                              window.SetViewerPose(new_pose);
                                                            });
      }
    }
};

static void ShowImage(const char * const model_filename)
{
  rotation_data data(model_filename);
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    std::cout << "Illustrates rotation in three dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << std::endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
  ShowImage(model_filename);
  return 0;
}
