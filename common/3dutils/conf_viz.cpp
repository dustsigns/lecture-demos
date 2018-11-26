//3-D visualization window with accompanying configuration window
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <opencv2/highgui.hpp>

#include "conf_viz.hpp"

using namespace std;

using namespace cv;
using namespace cv::viz;

namespace vizutils
{
  ConfigurableVisualization::WindowControl::WindowControl(ControlCallback callback, const int max_parameter, const int min_parameter, const int default_parameter, ConfigurableVisualization &parent)
   : callback(callback), max_parameter(max_parameter), min_parameter(min_parameter), parameter(default_parameter), parent(parent)
  {
    assert(min_parameter <= max_parameter);
    assert(default_parameter >= min_parameter && default_parameter <= max_parameter);
  }
  
  ConfigurableVisualization::ConfigurableVisualization(const string &visualization_window_name, const string &control_window_name)
   : visualization_window_name(visualization_window_name), control_window_name(control_window_name),
     visualization(visualization_window_name) { }
  
  ConfigurableVisualization::~ConfigurableVisualization()
  {
    visualization.close(); //Explicitly close 3-D visualization window
  }

  void ConfigurableVisualization::AddTrackbar(const string &name, ControlCallback callback, const int max_value, const int min_value, const int default_value)
  {
    controls.insert(make_pair(name, WindowControl(callback, max_value, min_value, default_value, *this)));
  }
  
  int ConfigurableVisualization::GetTrackbarValue(const string &name) const
  {
    const auto &trackbar = controls.at(name);
    return trackbar.parameter;
  }
  
  void ConfigurableVisualization::UpdateTrackbarValue(const string &name, const int value)
  {
    setTrackbarPos(name, control_window_name, value);
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
    namedWindow(control_window_name, WINDOW_AUTOSIZE | WINDOW_KEEPRATIO | WINDOW_GUI_NORMAL);
    for (auto &control_pair : controls)
    {
      const auto &control_name = control_pair.first;
      auto &control = control_pair.second;
      createTrackbar(control_name, control_window_name, &control.parameter, control.max_parameter, [](const int, void * const userdata)
                                                                                                     {
                                                                                                       auto &window_control = *(WindowControl * const)userdata;
                                                                                                       window_control.callback(window_control.parent);
                                                                                                     }, (void*)&control);
      setTrackbarMin(control_name, control_window_name, control.min_parameter);
      setTrackbarMax(control_name, control_window_name, control.max_parameter);
    }
    Mat empty(1, window_width, CV_8UC1, Scalar(0)); //TODO: Find a better way than an "empty" image to show a trackbar
    cv::imshow(control_window_name, empty);
  }

  void ConfigurableVisualization::AlignWindows(function<ViewerTransform> transform)
  {
    visualization.setWindowPosition(Point2i(0, 0));
    visualization.setWindowSize(Point2i(window_width, window_height));
    moveWindow(control_window_name, 0, window_height + 50); //Move window right below visualization (window size plus additional space for title and border)
    if (transform)
    {
      const auto old_pose = visualization.getViewerPose();
      const auto start_pose = transform(old_pose);
      visualization.setViewerPose(start_pose);
    }
  }
  
  void ConfigurableVisualization::ShowWindows(function<ViewerTransform> transform)
  {
    ShowVisualizationWindow();
    ShowControlWindow();
    visualization.spinOnce(1, true); //Needs to be called before accessing camera parameters
    AlignWindows(transform);
    while (!visualization.wasStopped())
    {
      if (waitKey(1) != -1)
        break;
      visualization.spinOnce(1, true);
    } //TODO: Figure out why programs using this function crash with a SIGSEV at the end due to the combined use of waitKey and spinOnce
  }
  
  Camera ConfigurableVisualization::GetCamera() const
  {
    return visualization.getCamera();
  }
  
  void ConfigurableVisualization::SetCamera(const Camera &camera)
  {
    visualization.setCamera(camera);
  }
  
  Affine3d ConfigurableVisualization::GetViewerPose() const
  {
    return visualization.getViewerPose();
  }
  
  void ConfigurableVisualization::SetViewerPose(const Affine3d &pose)
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
