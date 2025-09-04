//Multi-window arrangements (header)
// Andreas Unterweger, 2022-2025
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <vector>

#include <opencv2/core.hpp>

#include "window.hpp"

namespace imgutils
{
  //Position of windows in a group of windows
  enum class WindowAlignment { Horizontal, Vertical };
  
  //Represents an aligned group of multiple windows
  class MultiWindow : public VisibleWindow
  {
    public:
      //Creates a new group of windows from the given windows with the specified alignment
      MultiWindow(const std::vector<VisibleWindow*> &windows, const WindowAlignment alignment, const std::vector<VisibleWindow*> &hidden_windows = {});
      //Hides the window group
      ~MultiWindow() override;
      
      //The set of windows which will not be shown when Show() of the whole group is called
      std::vector<VisibleWindow*> hidden_windows;

      //Returns the total width and height of the group of windows
      cv::Size GetSize() const override;
      //Throws an exception as the size of a group of windows cannot be set in a meaningful way
      void SetSize(const cv::Size &size) override;
      
      //Sets the top-left position of the group of windows, thereby updating the positions of each individual window
      void SetPosition(const cv::Point &position) override;

      //Updates the window group (i.e., all windows of the group) if it is visible and set its position
      void Update(const bool first_update = false) override;
      
      //Throws an exception
      int Wait(const int timeout = 0) override;
      //Waits for the minimum amount of time (1) and returns the key pressed in any of the windows of the window group during this time, if any, or -1 otherwise.
      int WaitMinimal() override;
    protected:
      //The windows to be aligned
      std::vector<VisibleWindow*> windows;
      //How the windows are aligned
      const WindowAlignment alignment;
      
      //Creates the window group, i.e., it shows the aligned windows except for the ones which are to remain hidden
      void CreateWindow() override;
      //Does nothing
      void AfterCreateWindow() override;
      //Destroys the window group, i.e., it hides the aligned windows, including the ones which have not been shown as they remained hidden
      void DestroyWindow() override;
      
      //Creates a new empty group of windows with the specified alignment. The group needs to be initialized by SetWindows later
      MultiWindow(const WindowAlignment alignment);
      //Sets the specified groups of windows
      void SetWindows(const std::vector<VisibleWindow*> &windows);
  };
}
