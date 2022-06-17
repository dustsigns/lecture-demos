//Configurable visualization window abstraction
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include "confviz.hpp"

namespace vizutils
{
  ConfigurableVisualizationWindow::ConfigurableVisualizationWindow(const std::string &visualization_window_title, const std::string &configuration_window_title, const imgutils::WindowAlignment alignment)
    : MultiWindow(alignment),
      visualization_window(visualization_window_title),
      configuration_window(configuration_window_title)
  {
    const cv::Size default_size(VisualizationWindow::default_window_width, VisualizationWindow::default_window_height);
    SetSize(default_size);
    SetWindows({&visualization_window, &configuration_window});
  }
  
  void ConfigurableVisualizationWindow::SetSize(const cv::Size &size)
  {
    visualization_window.SetSize(size);
    const cv::Mat dummy_content(1, size.width, CV_8UC1, cv::Scalar(0)); //TODO: Find a better way than an "empty" image to show window controls
    configuration_window.UpdateContent(dummy_content);
    Update(); //Update positions
  }
}
