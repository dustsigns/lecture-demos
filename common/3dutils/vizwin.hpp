//Visualization window abstraction (header)
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <memory>
#include <vector>
#include <set>

#include <opencv2/core.hpp>
#include <opencv2/viz.hpp>

#include "window.hpp"

namespace vizutils
{
  //Represents a visualization window with its objects
  class VisualizationWindow : public imgutils::VisibleWindow
  {
    public:
      //Creates a new window with the given window title
      VisualizationWindow(const std::string &title);
      //Hides the visualization window
      ~VisualizationWindow() override;
      
      //Adds a widget to the visualization window. The name of the widget must be unique within the window
      void AddWidget(const std::string &name, cv::viz::Widget * const widget);
      //Removes a widget from the visualization window based on its name
      void RemoveWidget(const std::string &name);
      //Removes all widgets from the visualization window
      void ClearWidgets();
      
      //Returns the width and height of the window (including any borders and controls)
      cv::Size GetSize() const override;
      //Sets the width and height of the window
      void SetSize(const cv::Size &size) override;
      
      //Returns the position of the window
      cv::Point GetPosition() const override;
      //Sets the position of the window
      void SetPosition(const cv::Point &position) override;
      
      //Updates the visualization window if it is visible and sets its position and size
      void Update(const bool first_update = false) override;
      
      //Wait for the specified timeout if the window is shown (0 means infinite). The waiting process can be aborted by a key press in the window. In this case, 0 is returned, -1 otherwise.
      int Wait(const int timeout = 0) override;
      
      //Returns the visualization window's camera object
      cv::viz::Camera GetCamera() const;
      //Replaces the visualization window's camera object with the one specified
      void SetCamera(const cv::viz::Camera &camera);
      //Returns the visualization window's camera pose
      cv::Affine3d GetViewerPose() const;
      //Replaces the visualization window's camera pose with the one specified
      void SetViewerPose(const cv::Affine3d &pose);
      
      static constexpr auto default_window_width = 800;
      static constexpr auto default_window_height = 600;
    protected:
      //The underlying visualization window
      std::unique_ptr<cv::viz::Viz3d> visualization;
      //The names of all widgets that have been added to the shown visualization window
      std::set<std::string> widget_names;
      //The widgets that will be added after the visualization window is shown
      std::vector<std::pair<std::string, cv::viz::Widget*>> queued_widgets;
      
      //Removes a widget from the visualization window based on its iterator
      void RemoveVisibleWidget(const decltype(widget_names)::iterator &iterator);
      //Removes a widget from the visualization window based on its iterator
      void RemoveQueuedWidget(const decltype(queued_widgets)::iterator &iterator);
      
      //Creates the window
      void CreateWindow() override;
      //Adds widgets to the window
      void AfterCreateWindow() override;
      //Destroys the window on hiding
      void DestroyWindow() override;
  };
}
