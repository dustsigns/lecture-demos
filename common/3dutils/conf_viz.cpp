//3-D visualization window with accompanying configuration window
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <opencv2/highgui.hpp>

#include "conf_viz.hpp"

namespace vizutils
{
  ConfigurableVisualization::WindowControl::WindowControl(ControlCallback callback, const int max_parameter, const int min_parameter, const int default_parameter, ConfigurableVisualization &parent)
   : callback(callback), max_parameter(max_parameter), min_parameter(min_parameter), default_parameter(default_parameter), parent(parent)
  {
    assert(min_parameter <= max_parameter);
    assert(default_parameter >= min_parameter && default_parameter <= max_parameter);
  }
  
  ConfigurableVisualization::ConfigurableVisualization(const std::string &visualization_window_name, const std::string &control_window_name)
   : visualization_window_name(visualization_window_name), control_window_name(control_window_name),
     visualization(visualization_window_name),
     ready(false) { }
  
  ConfigurableVisualization::~ConfigurableVisualization()
  {
    visualization.close(); //Explicitly close 3-D visualization window
  }

  void ConfigurableVisualization::AddTrackbar(const std::string &name, ControlCallback callback, const int max_value, const int min_value, const int default_value)
  {
    controls.insert(make_pair(name, WindowControl(callback, max_value, min_value, default_value, *this)));
  }
  
  int ConfigurableVisualization::GetTrackbarValue(const std::string &name) const
  {
    const auto value = cv::getTrackbarPos(name, control_window_name);
    return value;
  }
  
  void ConfigurableVisualization::UpdateTrackbarValue(const std::string &name, const int value)
  {
    cv::setTrackbarPos(name, control_window_name, value);
  }

  void ConfigurableVisualization::ShowVisualizationWindow()
  {
    visualization.setBackgroundColor();
    for (const auto &object : objects)
      visualization.showWidget(object.first, object.second);
    visualization.spinOnce(1, true);
  }
 
  void ConfigurableVisualization::ShowControlWindow()
  {
    cv::namedWindow(control_window_name, cv::WINDOW_AUTOSIZE | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_NORMAL);
    for (auto &control_pair : controls)
    {
      const auto &control_name = control_pair.first;
      auto &control = control_pair.second;
      cv::createTrackbar(control_name, control_window_name, nullptr, control.max_parameter, [](const int, void * const userdata)
                                                                                              {
                                                                                                auto &window_control = *(static_cast<WindowControl*>(userdata));
                                                                                                auto &parent = window_control.parent;
                                                                                                if (parent.ready) //Only call callback after windows have been shown
                                                                                                  window_control.callback(parent);
                                                                                              }, static_cast<void*>(&control));
      cv::setTrackbarMin(control_name, control_window_name, control.min_parameter);
      cv::setTrackbarMax(control_name, control_window_name, control.max_parameter);
      cv::setTrackbarPos(control_name, control_window_name, control.default_parameter);
    }
    cv::Mat empty(1, window_width, CV_8UC1, cv::Scalar(0)); //TODO: Find a better way than an "empty" image to show a trackbar
    cv::imshow(control_window_name, empty);
  }

  void ConfigurableVisualization::AlignWindows(std::function<ViewerTransform> transform)
  {
    visualization.setWindowPosition(cv::Point2i(0, 0));
    visualization.setWindowSize(cv::Point2i(window_width, window_height));
    cv::moveWindow(control_window_name, 0, window_height + 50); //Move window right below visualization (window size plus additional space for title and border)
    if (transform)
    {
      const auto old_pose = visualization.getViewerPose();
      const auto start_pose = transform(old_pose);
      visualization.setViewerPose(start_pose);
    }
  }
  
  void ConfigurableVisualization::ShowWindows(std::function<ViewerTransform> transform, ConfigurableVisualization::ControlCallback initial_callback)
  {
    ShowVisualizationWindow();
    ShowControlWindow();
    visualization.spinOnce(1, true); //Needs to be called before accessing camera parameters
    AlignWindows(transform);
    if (initial_callback)
      initial_callback(*this);
    ready = true;
    while (!visualization.wasStopped())
    {
      if (cv::waitKey(1) != -1)
        break;
      visualization.spinOnce(1, true);
    }
  }
  
  cv::viz::Camera ConfigurableVisualization::GetCamera() const
  {
    return visualization.getCamera();
  }
  
  void ConfigurableVisualization::SetCamera(const cv::viz::Camera &camera)
  {
    visualization.setCamera(camera);
  }
  
  cv::Affine3d ConfigurableVisualization::GetViewerPose() const
  {
    return visualization.getViewerPose();
  }
  
  void ConfigurableVisualization::SetViewerPose(const cv::Affine3d &pose)
  {
    visualization.setViewerPose(pose);
  }
  
  void ConfigurableVisualization::ClearObjects()
  {
    for (const auto &name_and_object : objects)
      visualization.removeWidget(name_and_object.first); //Remove widgets by name (key)
    objects.clear();
  }
  
  void ConfigurableVisualization::RedrawObjects()
  {
    ShowVisualizationWindow();
  }
}
