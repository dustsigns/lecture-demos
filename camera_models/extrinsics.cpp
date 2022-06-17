//Illustration of extrinsic camera parameters
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <memory>
#include <algorithm>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "math.hpp"
#include "confviz.hpp"

class extrinsics_data
{
  protected:
    static constexpr char axes[] = {'x', 'y', 'z'};
    static constexpr char last_axis = axes[comutils::arraysize(axes) - 1];
    
    vizutils::ConfigurableVisualizationWindow configurable_visualization;
    
    using TrackBarType = imgutils::TrackBar<extrinsics_data&>;
    std::unique_ptr<TrackBarType> rotation_trackbars[comutils::arraysize(axes)];
    std::unique_ptr<TrackBarType> translation_trackbars[comutils::arraysize(axes)];
    
    std::unique_ptr<cv::viz::Widget> object;
    
    static constexpr auto parameter_accuracy = 0.01;
    
    void AddObject(const char * const model_filename)
    {
      if (model_filename)
      {
        const auto mesh = cv::viz::Mesh::load(model_filename);
        object = std::make_unique<cv::viz::WMesh>(mesh);
      }
      else
      {
        constexpr auto cone_length = 1.0;
        constexpr auto cone_radius = 0.5;
        constexpr auto cone_resolution = 100;  
        object = std::make_unique<cv::viz::WCone>(cone_length, cone_radius, cone_resolution);
      }
      configurable_visualization.visualization_window.AddWidget("Original object", object.get());
    }
    
    static void UpdateCameraPose(extrinsics_data &data)
    {
      double rotation_angles[comutils::arraysize(axes)];
      assert(comutils::arraysize(axes) == comutils::arraysize(data.rotation_trackbars));
      std::transform(std::begin(data.rotation_trackbars), std::end(data.rotation_trackbars), std::begin(rotation_angles),
                     [](const std::unique_ptr<TrackBarType> &trackbar)
                       {
                         const auto rotation_degrees = trackbar->GetValue();
                         const auto rotation = comutils::DegreesToRadians(rotation_degrees);
                         return rotation;
                       });
      double translation_offsets[comutils::arraysize(axes)];
      assert(comutils::arraysize(axes) == comutils::arraysize(data.translation_trackbars));
      std::transform(std::begin(data.translation_trackbars), std::end(data.translation_trackbars), std::begin(translation_offsets),
                     [](const std::unique_ptr<TrackBarType> &trackbar)
                       {
                         const auto translation_unscaled = trackbar->GetValue();
                         const auto translation = translation_unscaled * parameter_accuracy;
                         return translation;
                       });      
      const cv::Vec3d rotation(rotation_angles);
      const cv::Vec3d translation(translation_offsets);
      const cv::Affine3d pose(rotation, translation);
      const cv::Affine3d rounded_pose = RoundCameraPose(pose);
      data.configurable_visualization.visualization_window.SetViewerPose(rounded_pose);
    }
    
    void AddControls()
    {
      std::transform(std::begin(axes), std::end(axes), std::begin(rotation_trackbars),
                     [this](const char axis)
                           {
                             using namespace std::string_literals;
                             const auto trackbar_name = "Rotation ("s + axis + ") [°]";
                             return std::make_unique<TrackBarType>(trackbar_name, configurable_visualization.configuration_window, 360, 0, 0, UpdateCameraPose, *this);
                           });
      std::transform(std::begin(axes), std::end(axes), std::begin(translation_trackbars),
                     [this](const char axis)
                           {
                             using namespace std::string_literals;
                             const auto trackbar_name = "Translation ("s + axis + ") [px]";
                             const auto limit = axis == last_axis ? 500: 50; //Different range for z axis
                             return std::make_unique<TrackBarType>(trackbar_name, configurable_visualization.configuration_window, limit, -limit, 0, UpdateCameraPose, *this);
                           });
    }
    
    static cv::Affine3d RoundCameraPose(const cv::Affine3d &old_pose)
    {
      cv::Vec3d rotation(old_pose.rvec());
      for (size_t i = 0; i < 3; i++)
        rotation[i] = floor(rotation[i] / parameter_accuracy) * parameter_accuracy;
        
      cv::Vec3d translation(old_pose.translation());
      for (size_t i = 0; i < 3; i++)
        translation[i] = floor(translation[i] / parameter_accuracy) * parameter_accuracy;
        
      const cv::Affine3d pose(rotation, translation);
      return pose;
    }
    
    void InitializeControlValues(const cv::Affine3d &pose)
    {
      const auto rotation = pose.rvec();
      assert(comutils::arraysize(axes) == comutils::arraysize(rotation_trackbars));
      for (size_t i = 0; i < comutils::arraysize(axes); i++)
      {
        const auto rotation_radians = comutils::RadiansToDegrees(rotation[i]);
        rotation_trackbars[i]->SetValue(rotation_radians);
      }
      const auto translation = pose.translation();
      assert(comutils::arraysize(axes) == comutils::arraysize(translation_trackbars));
      for (size_t i = 0; i < comutils::arraysize(axes); i++)
      {
        const auto translation_scaled = translation[i] / parameter_accuracy;
        translation_trackbars[i]->SetValue(translation_scaled);
      }
    }
  
    static constexpr auto visualization_window_name = "Camera view";
    static constexpr auto control_window_name = "Extrinsic camera parameters";
  public:
    extrinsics_data(const char * const model_filename)
     : configurable_visualization(visualization_window_name, control_window_name)
    {
      AddObject(model_filename);
      AddControls();
    }
  
    void ShowImage()
    {
      configurable_visualization.ShowInteractive([this]()
                                                       {
                                                         const auto pose = configurable_visualization.visualization_window.GetViewerPose();
                                                         const cv::Affine3d rounded_pose = RoundCameraPose(pose);
                                                         InitializeControlValues(rounded_pose);
                                                         configurable_visualization.visualization_window.SetViewerPose(rounded_pose);
                                                       });
    }
};



void ShowImage(const char * const model_filename)
{
  extrinsics_data data(model_filename);
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    std::cout << "Illustrates the effect of the extrinsic parameters of a pinhole camera." << std::endl;
    std::cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << std::endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
  ShowImage(model_filename);
  return 0;
}
