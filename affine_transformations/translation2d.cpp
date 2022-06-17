//Illustration of 2-D translation
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>

#include "common.hpp"
#include "confviz.hpp"

class translation_data
{
  protected:
    static constexpr char axes[] = {'X', 'Y'};
    
    vizutils::ConfigurableVisualizationWindow configurable_visualization;
      
    using TrackBarType = imgutils::TrackBar<translation_data&>;
    std::unique_ptr<TrackBarType> translation_trackbars[comutils::arraysize(axes)];
    
    std::unique_ptr<cv::viz::Widget3D> coordinate_system;
    std::unique_ptr<cv::viz::Widget3D> original_object;
    std::unique_ptr<cv::viz::Widget3D> transformed_object;
    
    static constexpr auto letter_size = 0.1;
    
    void AddCoordinateSystem()
    {
      coordinate_system = std::make_unique<cv::viz::WCoordinateSystem>(4 * letter_size);
      configurable_visualization.visualization_window.AddWidget("Coordinate system", coordinate_system.get());
    }
    
    void AddObjects()
    {
      constexpr auto text = "A";
      original_object = std::make_unique<cv::viz::WText3D>(text, cv::Point3d(0, letter_size, 0), letter_size);
      transformed_object = std::make_unique<cv::viz::WText3D>(text, cv::Point3d(0, letter_size, 0), letter_size);
      original_object->setRenderingProperty(cv::viz::OPACITY, 0.5);
      configurable_visualization.visualization_window.AddWidget("Original object", original_object.get());
      configurable_visualization.visualization_window.AddWidget("Transformed object", transformed_object.get());
    }
    
    static void UpdateImage(translation_data &data)
    {
      double offsets[comutils::arraysize(axes)];
      assert(comutils::arraysize(axes) == comutils::arraysize(data.translation_trackbars));
      std::transform(std::begin(data.translation_trackbars), std::end(data.translation_trackbars), std::begin(offsets),
                     [](const std::unique_ptr<TrackBarType> &trackbar)
                       {
                         const auto offset_percent = trackbar->GetValue();
                         const auto offset = offset_percent / 100.0;
                         return offset;
                       });
      const cv::Vec3d offset(offsets);
      const auto transformation = cv::Affine3d::Identity().translate(offset);
      data.transformed_object->setPose(transformation);
    }
    
    void AddControls()
    {
      std::transform(std::begin(axes), std::end(axes), std::begin(translation_trackbars),
                     [this](const auto axis)
                           {
                             using namespace std::string_literals;
                             const auto trackbar_name = ""s + axis + " offset";
                             return std::make_unique<TrackBarType>(trackbar_name, configurable_visualization.configuration_window, 50, -50, 0, UpdateImage, *this);
                           });
    }
    
    static constexpr auto visualization_window_name = "2-D translation";
    static constexpr auto control_window_name = "2-D translation parameters";
  public:
    translation_data()
     : configurable_visualization(visualization_window_name, control_window_name)
    {
      AddCoordinateSystem();
      AddObjects();
      AddControls();
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
    std::cout << "Illustrates translation in two dimensions." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  translation_data data;
  data.ShowImage();
  return 0;
}
