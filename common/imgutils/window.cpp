//Window abstraction
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <stdexcept>
#include <algorithm>

#include <opencv2/highgui.hpp>

#include "window.hpp"

namespace imgutils
{
  WindowBase::WindowBase() : size(cv::Size()), position(cv::Point()) { }

  cv::Size WindowBase::GetSize() const
  {
    return size;
  }
  
  void WindowBase::SetSize(const cv::Size &size)
  {
    this->size = size;
  }
  
  cv::Point WindowBase::GetPosition() const
  {
    return position;
  }
  
  void WindowBase::SetPosition(const cv::Point &position)
  {
    this->position = position;
  }
  
  VisibleWindow::VisibleWindow(const std::string &title)
    : title(title),
      shown(false)
  { }
  
  VisibleWindow::~VisibleWindow()
  { }
  
  void VisibleWindow::Show()
  {
    const bool first_show = !shown;
    if (first_show) //Don't recreate a window if it is already shown
    {
      CreateWindow();
      shown = true;
      AfterCreateWindow();
    }
    Update(first_show);
  }
  
  void VisibleWindow::Hide()
  {
    if (!shown) //Only destroy a window if has not been shown
      return;
    shown = false;
    DestroyWindow();
  }
  
  bool VisibleWindow::IsShown() const
  {
    return shown;
  }
  
  int VisibleWindow::ShowInteractive(const std::function<void()> &after_show_callback, const int wait_time, const bool hide_after)
  {
    Show();
    if (after_show_callback)
      after_show_callback();
    int ret;
    if (wait_time == 0) //Infinite waiting
      while ((ret = WaitMinimal()) == -1);
    else
      ret = Wait(wait_time);
    if (hide_after)
      Hide();
    return ret;
  }
  
  int VisibleWindow::WaitMinimal()
  {
    return Wait(1);
  }
  
  cv::Size VisibleWindow::GetTotalSize(const cv::Size &size)
  {
    const cv::Size borders = cv::Size(2 * (outer_border_size + inner_border_size), outer_border_size + title_bar_height + inner_border_size + outer_border_size); //Left and right borders, top title bar and bottom border
    return size + borders;
  }
  
  Window::Window(const std::string &title, const cv::Mat &content, const cv::Size &size)
    : VisibleWindow(title),
      content(content),
      always_show_enhanced(false), position_like_enhanced(false)
  {
    SetSize(size);
  }
  
  Window::~Window()
  {
    Hide();
  }
  
  void Window::AddControl(WindowControlBase * const control, const bool requires_hidden_window)
  {
    if (requires_hidden_window && shown)
      throw std::runtime_error("Controls cannot be added while the window is shown");
    if (std::any_of(controls.begin(), controls.end(),
        [control](WindowControlBase * const ctrl)
                 {
                   return ctrl->name == control->name;
                 }))
      throw std::runtime_error("A control with the name " + control->name + " already exists in this window");
    controls.push_back(control);
  }

  void Window::RemoveControl(WindowControlBase * const control, const bool requires_hidden_window)
  {
    if (requires_hidden_window && shown)
      throw std::runtime_error("Controls cannot be removed while the window is shown");
    controls.erase(std::remove(controls.begin(), controls.end(), control), controls.end());
  }
  
  cv::Size Window::GetContentSize() const
  {
    const auto content_size = size == cv::Size() ? content.size() : size; //Image size (if size is set to zeros), or desired size
    return content_size;
  }
  
  int Window::GetControlHeights() const
  {
    return std::accumulate(controls.begin(), controls.end(), 0, [](const int sum, const WindowControlBase * const control)
                                                                  {
                                                                    return sum + control->GetHeight();
                                                                  });
  }
  
  cv::Size Window::GetSize() const
  {
    const auto content_size = GetContentSize();
    auto total_size = VisibleWindow::GetTotalSize(content_size);
    if (IsEnhanced() || position_like_enhanced) //Enhanced window
      total_size.height += tool_bar_height + status_bar_height; //Consider additional tool bar and status bar
    total_size.height += GetControlHeights(); //Consider heights of window controls
    return total_size;
  }
  
  void Window::SetSize(const cv::Size &size)
  {
    VisibleWindow::SetSize(size);
    if (shown)
    {
      auto actual_size = GetContentSize();
      if (IsEnhanced())
        actual_size.height += tool_bar_height + status_bar_height;
      actual_size.height += GetControlHeights();
      cv::resizeWindow(title, actual_size); //In enhanced windows, OpenCV uses this size for the total inner window size (including tool bars etc.), not only the content, so add everything that contributes to this size so that the content is of the specified size
    }
  }

  void Window::Zoom(const double zoom_factor)
  {
    if (zoom_factor <= 0)
      throw std::runtime_error("The zoom factor must be larger than zero");
    const auto size = content.size();
    const auto zoomed_size = cv::Size(static_cast<int>(size.width * zoom_factor), static_cast<int>(size.height * zoom_factor));
    SetSize(zoomed_size);
  }
  
  void Window::ZoomFully()
  {
    constexpr auto zoom_factor = 30; //OpenCV-specific
    Zoom(zoom_factor);
  }
  
  cv::Point Window::GetPosition() const
  {
    auto actual_position = position;
    actual_position.y += desktop_offset_height;
    if (position_like_enhanced)
      actual_position.y += tool_bar_height; //Position the window as if it had a tool bar on top
    return actual_position;
  }
  
  void Window::SetPosition(const cv::Point &position)
  {
    VisibleWindow::SetPosition(position);
    if (shown)
    {
      const auto actual_position = GetPosition();
      cv::moveWindow(title, actual_position.x, actual_position.y);
    }
  }
  
  void Window::SetAlwaysShowEnhanced(const bool always_show_enhanced)
  {
    this->always_show_enhanced = always_show_enhanced;
  }
  
  void Window::SetPositionLikeEnhanced(const bool position_like_enhanced)
  {
    this->position_like_enhanced = position_like_enhanced;
  }
  
  void Window::CreateWindow()
  {
    int flags = cv::WINDOW_KEEPRATIO;
    if (IsEnhanced())
      flags |= cv::WINDOW_GUI_EXPANDED; //Enhanced UI if any control requires it
    else
      flags |= cv::WINDOW_GUI_NORMAL; //Simple UI if there are no controls to be shown
    cv::namedWindow(title, flags);
    
    for (auto &control : controls)
    {
      if (control->requires_hidden_window)
        control->Render();
    }
  }
  
  void Window::AfterCreateWindow()
  {
    for (auto &control : controls)
    {
      if (!control->requires_hidden_window)
        control->Render();
    }
  }
  
  void Window::Update(const bool first_update)
  {
    if (shown)
    {
      cv::imshow(title, content);
      if (first_update)
        Wait(10); //Wait for a short while after showing a window for the first time; if there is no waiting or if the time is much shorter, e.g., 1 ms, sometimes windows are not positioned correctly despite the position below being correct. This happens especially when windows have been hidden before //TODO: Is this a bug in the window manager or can it be fixed in another way?
    }
    SetPosition(position);
    SetSize(size);
  }
  
  void Window::DestroyWindow()
  {
    cv::destroyWindow(title);
  }
  
  int Window::Wait(const int timeout)
  {
    if (!shown)
      throw std::runtime_error("Waiting is only possible when the window is shown");
    return cv::waitKey(timeout);
  }
  
  void Window::ShowOverlayText(const std::string &text, const bool subtle, const int timeout)
  {
    if (!shown)
      throw std::runtime_error("An overlay can only be shown when the window is shown");
    if (!IsEnhanced())
      throw std::runtime_error("Overlays can only be shown in enhanced windows");
    if (!subtle)
      cv::displayOverlay(title, text, timeout);
    cv::displayStatusBar(title, text);
  }
  
  bool Window::IsEnhanced() const
  {
    return always_show_enhanced || std::any_of(controls.begin(), controls.end(),
                                               [](const WindowControlBase * const control)
                                                 {
                                                   return control->RequiresEnhancedWindow();
                                                 });
  }
  
  void Window::UpdateContent(const cv::Mat &content)
  {
    this->content = content;
    Update();
  }
}
