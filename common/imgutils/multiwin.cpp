//Multi-window arrangements
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cassert>

#include "multiwin.hpp"

namespace imgutils
{
  MultiWindow::MultiWindow(const std::vector<VisibleWindow*> &windows, const WindowAlignment alignment, const std::vector<VisibleWindow*> &hidden_windows)
   : VisibleWindow("Window group"), hidden_windows(hidden_windows), alignment(alignment)
  {
    SetWindows(windows);
  }
  
  MultiWindow::MultiWindow(const WindowAlignment alignment)
   : VisibleWindow("Protected window group"), alignment(alignment)
  { }
  
  MultiWindow::~MultiWindow()
  {
    Hide();
  }

  void MultiWindow::SetWindows(const std::vector<VisibleWindow*> &windows)
  {
    assert(windows.size() >= 1);
    for (const auto * const window : windows)
      assert(window); //Window must not be null
    this->windows = windows;
    SetPosition(position);
  }
  
  cv::Size MultiWindow::GetSize() const
  {
    //Find maximum height when aligning horizontally, or maximum height when aligning vertically. Then add up widths for horizontal alignment, or heights for vertical alignment
    switch (alignment)
    {
      case WindowAlignment::Horizontal:
        {
          const auto max_height_window = *std::max_element(windows.begin(), windows.end(),
                                                           [](VisibleWindow * const first, VisibleWindow * const second)
                                                             {
                                                               const auto first_size = first->GetSize();
                                                               const auto second_size = second->GetSize();
                                                               return first_size.height < second_size.height;
                                                             });
          const auto max_height = max_height_window->GetSize().height;
          return std::accumulate(windows.begin(), windows.end(), cv::Size(),
                                 [max_height](const cv::Size &old_size, VisibleWindow * const window)
                                             {
                                               const auto new_size = window->GetSize();
                                               return cv::Size(old_size.width + new_size.width, max_height);
                                             });
          break;
        }
      case WindowAlignment::Vertical:
        {
          const auto max_width_window = *std::max_element(windows.begin(), windows.end(),
                                                          [](VisibleWindow * const first, VisibleWindow * const second)
                                                            {
                                                              const auto first_size = first->GetSize();
                                                              const auto second_size = second->GetSize();
                                                              return first_size.width < second_size.width;
                                                            });
          const auto max_width = max_width_window->GetSize().width;
          return std::accumulate(windows.begin(), windows.end(), cv::Size(),
                                 [max_width](const cv::Size &old_size, VisibleWindow * const window)
                                            {
                                              const auto new_size = window->GetSize();
                                              return cv::Size(max_width, old_size.height + new_size.height);
                                            });
          break;
        }
      default:
        return cv::Size(); //Invalid case
    }
  }
  
  void MultiWindow::SetSize(const cv::Size&)
  {
    throw std::runtime_error("The size of a group of windows cannot be set");
  }
  
  void MultiWindow::SetPosition(const cv::Point &position)
  {
    VisibleWindow::SetPosition(position);
    cv::Point current_position = position;
    for (auto * const window : windows)
    {
      window->SetPosition(current_position);
      const auto window_size = window->GetSize();
      switch (alignment)
      {
        case WindowAlignment::Horizontal:
          current_position += cv::Point(window_size.width, 0);
          break;
        case WindowAlignment::Vertical:
          current_position += cv::Point(0, window_size.height);
          break;
      }
    }
  }

  void MultiWindow::CreateWindow()
  {
    for (auto * const window : windows)
    {
      const bool show = std::find(hidden_windows.begin(), hidden_windows.end(), window) == hidden_windows.end();
      if (show) //Only show the window if it cannot be found in the list of hidden windows
        window->Show();
    }
  }
  
  void MultiWindow::AfterCreateWindow()
  {
  }
  
  void MultiWindow::Update(const bool)
  {
    SetPosition(position);
  }
  
  void MultiWindow::DestroyWindow()
  {
    for (auto * const window : windows)
      window->Hide();
  }
  
  int MultiWindow::Wait(const int)
  {
    throw std::runtime_error("Wait is not supported for window groups");
  }
  
  int MultiWindow::WaitMinimal()
  {
    for (auto * const window : windows)
    {
      const bool wait = std::find(hidden_windows.begin(), hidden_windows.end(), window) == hidden_windows.end();
      int ret;
      if (wait && (ret = window->WaitMinimal()) != -1)
        return ret;
    }
    return -1;
  }
}
