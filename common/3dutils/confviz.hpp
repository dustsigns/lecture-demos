//Configurable visualization window abstraction (header)
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <memory>
#include <vector>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/viz.hpp>

#include "window.hpp"
#include "vizwin.hpp"
#include "multiwin.hpp"

namespace vizutils
{
  //Represents a configurable visualization window, consisting of a visualization window and a configuration-only window with window controls
  class ConfigurableVisualizationWindow : public imgutils::MultiWindow
  {
    public:
      //Creates a new window with the given window (part) titles and alignment
      ConfigurableVisualizationWindow(const std::string &visualization_window_title, const std::string &configuration_window_title, const imgutils::WindowAlignment alignment = imgutils::WindowAlignment::Vertical);
      
      //The visualization window (part)
      VisualizationWindow visualization_window;
      //The configuration window (part)
      imgutils::Window configuration_window;
      
      //Sets the width and height of the visualization window as well as the width of the configuration window
      void SetSize(const cv::Size &size) override;
  };
}
