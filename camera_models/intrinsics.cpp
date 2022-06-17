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

class intrinsics_data
{
  protected:
    static constexpr char components[] = {'x', 'y'};
    static constexpr int default_dimensions[] { vizutils::VisualizationWindow::default_window_width, vizutils::VisualizationWindow::default_window_height };
    static_assert(comutils::arraysize(components) == comutils::arraysize(default_dimensions), "There must be exactly as many default dimensions as there are components");
    
    vizutils::ConfigurableVisualizationWindow configurable_visualization;
    
    using TrackBarType = imgutils::TrackBar<intrinsics_data&>;
    std::unique_ptr<TrackBarType> focal_length_trackbars[comutils::arraysize(components)];
    std::unique_ptr<TrackBarType> principal_point_trackbars[comutils::arraysize(components)];
    
    std::unique_ptr<cv::viz::Widget> object;
    
    bool initialized;
    
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
    
    static void UpdateCamera(intrinsics_data &data)
    {
      if (!data.initialized) //Don't update the camera while the window and its controls have not been fully initialized
        return;
      double focal_lengths[comutils::arraysize(components)];
      std::transform(std::begin(data.focal_length_trackbars), std::end(data.focal_length_trackbars), std::begin(focal_lengths),
                     [](const std::unique_ptr<TrackBarType> &trackbar)
                       {
                         const auto focal_length = trackbar->GetValue();
                         return focal_length;
                       });
      double principal_point_components[comutils::arraysize(components)];
      std::transform(std::begin(data.principal_point_trackbars), std::end(data.principal_point_trackbars), std::begin(principal_point_components),
                     [](const std::unique_ptr<TrackBarType> &trackbar)
                       {
                         const auto principal_point_component = trackbar->GetValue();
                         return principal_point_component;
                       });
      auto &window = data.configurable_visualization.visualization_window;
      const auto old_camera = window.GetCamera();
      assert(comutils::arraysize(components) == 2);
      const cv::viz::Camera camera(focal_lengths[0], focal_lengths[1], principal_point_components[0], principal_point_components[1], old_camera.getWindowSize());
      window.SetCamera(camera);
    }
    
    void AddControls()
    {
      assert(comutils::arraysize(components) == comutils::arraysize(focal_length_trackbars));
      for (size_t i = 0; i < comutils::arraysize(components); i++)
      {
        using namespace std::string_literals;
        const auto &component = components[i];
        const auto trackbar_name = "Focal length ("s + component + ") [px]";
        const auto default_dimension = default_dimensions[i];
        focal_length_trackbars[i] = std::make_unique<TrackBarType>(trackbar_name, configurable_visualization.configuration_window, 2 * default_dimension, 0, 0, UpdateCamera, *this); //TODO: Find a more meaningful maximum value
      }
      assert(comutils::arraysize(components) == comutils::arraysize(principal_point_trackbars));
      for (size_t i = 0; i < comutils::arraysize(components); i++)
      {
        using namespace std::string_literals;
        const auto &component = components[i];
        const auto trackbar_name = "Image center ("s + component + ") [px]";
        const auto default_dimension = default_dimensions[i];
        principal_point_trackbars[i] = std::make_unique<TrackBarType>(trackbar_name, configurable_visualization.configuration_window, default_dimension, 0, 0, UpdateCamera, *this);
      }
    }
    
    void InitializeControlValues()
    {
      const auto camera = configurable_visualization.visualization_window.GetCamera();
      const auto focal_length = camera.getFocalLength();
      const auto focal_lengths = static_cast<cv::Vec2i>(focal_length);
      assert(comutils::arraysize(components) == decltype(focal_lengths)::channels);
      assert(comutils::arraysize(components) == comutils::arraysize(focal_length_trackbars));
      for (size_t i = 0; i < comutils::arraysize(components); i++)
        focal_length_trackbars[i]->SetValue(focal_lengths[i]);

      const auto principal_point = camera.getPrincipalPoint();
      assert(comutils::arraysize(components) == decltype(principal_point)::channels);
      assert(comutils::arraysize(components) == comutils::arraysize(principal_point_trackbars));
      for (size_t i = 0; i < comutils::arraysize(components); i++)
        principal_point_trackbars[i]->SetValue(principal_point[i]);
    }
  
    static constexpr auto visualization_window_name = "Camera view";
    static constexpr auto control_window_name = "Intrinsic camera parameters";
  public:
    intrinsics_data(const char * const model_filename)
     : configurable_visualization(visualization_window_name, control_window_name),
       initialized(false)
    {
      AddObject(model_filename);
      AddControls();
    }
  
    void ShowImage()
    {
      configurable_visualization.ShowInteractive([this]()
                                                       {
                                                         InitializeControlValues();
                                                         initialized = true;
                                                       });
    }
};



void ShowImage(const char * const model_filename)
{
  intrinsics_data data(model_filename);
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    std::cout << "Illustrates the effect of the intrinsic parameters of a pinhole camera." << std::endl;
    std::cout << "Usage: " << argv[0] << " [3-D model (PLY) file name]" << std::endl;
    return 1;
  }
  const auto model_filename = (argc == 2) ? argv[1] : nullptr;
  ShowImage(model_filename);
  return 0;
}
