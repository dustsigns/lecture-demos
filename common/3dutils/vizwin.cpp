//Visualization window abstraction
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <stdexcept>
#include <algorithm>
#include <limits>

#include <opencv2/viz.hpp>

#include "vizwin.hpp"

namespace vizutils
{
  VisualizationWindow::VisualizationWindow(const std::string &title)
    : imgutils::VisibleWindow(title)
  {
    SetSize(cv::Size(default_window_width, default_window_height));
  }
  
  VisualizationWindow::~VisualizationWindow()
  {
    Hide();
  }
  
  void VisualizationWindow::AddWidget(const std::string &name, cv::viz::Widget * const widget)
  {
    if (shown)
      {
      if (widget_names.find(name) != widget_names.end())
        throw std::runtime_error("A widget with the name " + name + " already exists in this visualization window");
      widget_names.insert(name);
      visualization->showWidget(name, *widget);
    }
    else
    {
      if (std::find_if(queued_widgets.begin(), queued_widgets.end(), [name](const std::pair<std::string, cv::viz::Widget*> &queued_widget)
                                                                         {
                                                                           return name == queued_widget.first;
                                                                         }) != queued_widgets.end())
        throw std::runtime_error("A widget with the name " + name + " already exists in the queue of this visualization window");
      queued_widgets.emplace_back(name, widget);
    }
  }
  
  void VisualizationWindow::RemoveWidget(const std::string &name)
  {
    if (shown)
    {
      const auto iterator = widget_names.find(name);
      if (iterator == widget_names.end())
        throw std::runtime_error("A widget with the name " + name + " does not exist in this visualization window");
      RemoveVisibleWidget(iterator);
    }
    else
    {
      const auto iterator = std::find_if(queued_widgets.begin(), queued_widgets.end(), [name](const std::pair<std::string, cv::viz::Widget*> &queued_widget)
                                                                                             {
                                                                                               return name == queued_widget.first;
                                                                                             });
      if (iterator == queued_widgets.end())
        throw std::runtime_error("A widget with the name " + name + " does not exist in the queue of this visualization window");
      RemoveQueuedWidget(iterator);
    }
  }
  
  void VisualizationWindow::RemoveVisibleWidget(const decltype(VisualizationWindow::widget_names)::iterator &iterator)
  {
    const auto &name = *iterator;
    visualization->removeWidget(name);
    widget_names.erase(iterator, widget_names.end());
  }

  void VisualizationWindow::RemoveQueuedWidget(const decltype(VisualizationWindow::queued_widgets)::iterator &iterator)
  {
    queued_widgets.erase(iterator, queued_widgets.end());
  }
  
  void VisualizationWindow::ClearWidgets()
  {
    if (shown)
    {
      while (!widget_names.empty()) //Delete from the back
        RemoveVisibleWidget(std::prev(widget_names.end()));
    }
    else
    {
      while (!queued_widgets.empty()) //Delete from the back
        RemoveQueuedWidget(std::prev(queued_widgets.end()));
    }
  }
  
  cv::Size VisualizationWindow::GetSize() const
  {
    const auto content_size = imgutils::VisibleWindow::GetSize();
    const auto total_size = VisibleWindow::GetTotalSize(content_size);
    return total_size;
  }
  
  void VisualizationWindow::SetSize(const cv::Size &size)
  {
    VisibleWindow::SetSize(size);
    if (shown)
      visualization->setWindowSize(size);
  }

  cv::Point VisualizationWindow::GetPosition() const
  {
    auto actual_position = position;
    if (position.y > 0)
      actual_position.y += title_bar_height - outer_border_size;  //OpenCV interprets the position to be that of the inner window content, unless the position is zero, so add the title bar height to show the window at its specified position
    if (position.y <= title_bar_height - outer_border_size)
      actual_position.y += title_bar_height - outer_border_size; //OpenCV treats all Y coordinates smaller than the beginning of the inner window content as zero, so add the offset again to position the window as specified
    return actual_position;
  }
  
  void VisualizationWindow::SetPosition(const cv::Point &position)
  {
    imgutils::VisibleWindow::SetPosition(position);
    if (shown)
    {
      const auto actual_position = GetPosition();
      visualization->setWindowPosition(actual_position);
    }
  }
  
  void VisualizationWindow::CreateWindow()
  {
    visualization = std::make_unique<cv::viz::Viz3d>(title);
  }
  
  void VisualizationWindow::AfterCreateWindow()
  {
    for (const auto& [name, widget] : queued_widgets)
    {
      AddWidget(name, widget);
    }
    queued_widgets.clear();
    Wait(1); //Make sure that the window is rendered for a least one ms so that camera properties etc. can be accessed and have meaningful values
  }

  void VisualizationWindow::Update(const bool)
  {
    SetPosition(position);
    SetSize(size);
  }
  
  void VisualizationWindow::DestroyWindow()
  {
    ClearWidgets();
    visualization->close();
  }
  
  int VisualizationWindow::Wait(const int timeout)
  {
    if (!shown)
      throw std::runtime_error("Waiting is only possible when the visualization window is shown");
    const auto actual_timeout = timeout == 0 ? std::numeric_limits<decltype(timeout)>::max() : timeout; //Wait for as long as possible if 0 is used as a placeholder for infinite waiting time
    visualization->spinOnce(actual_timeout, true);
    return visualization->wasStopped() ? 0 : -1;
  }

  cv::viz::Camera VisualizationWindow::GetCamera() const
  {
    if (!shown)
      throw std::runtime_error("Accessing the camera is only possible when the visualization window is shown");
    return visualization->getCamera();
  }
  
  void VisualizationWindow::SetCamera(const cv::viz::Camera &camera)
  {
    if (!shown)
      throw std::runtime_error("Accessing the camera is only possible when the visualization window is shown");
    visualization->setCamera(camera);
  }
  
  cv::Affine3d VisualizationWindow::GetViewerPose() const
  {
    if (!shown)
      throw std::runtime_error("Accessing the viewer pose is only possible when the visualization window is shown");
    return visualization->getViewerPose();
  }
  
  void VisualizationWindow::SetViewerPose(const cv::Affine3d &pose)
  {
    if (!shown)
      throw std::runtime_error("Accessing the viewer pose is only possible when the visualization window is shown");
    visualization->setViewerPose(pose);
  }
}
