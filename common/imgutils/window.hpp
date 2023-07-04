//Window abstraction (header)
// Andreas Unterweger, 2022-2023
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <vector>

#include <opencv2/core.hpp>

namespace imgutils { class WindowControlBase; } //Forward declaration required for window controls
//#include "winctrl.hpp" //Included below because it requires the full definition of the Window class

namespace imgutils
{
  //Represents an abstraction of a window-like construct
  class WindowBase
  {
    public:
      //Initializes the window size and position (to zeros)
      WindowBase();
    
      //Returns the width and height of the window (this may be larger than the originally set size in child classes)
      virtual cv::Size GetSize() const;
      //Sets the width and the height of the window (this may be larger than the originally set size in child classes)
      virtual void SetSize(const cv::Size &size);
      
      //Returns the position of the window
      virtual cv::Point GetPosition() const;
      //Sets the position of the window
      virtual void SetPosition(const cv::Point &position);
    
      //Displays the window (to be overwritten in derived classes)
      virtual void Show() = 0;
      //Hides the window (to be overwritten in derived classes)
      virtual void Hide() = 0;
    protected:
      //The size of the window
      cv::Size size;
      //The position of the window
      cv::Point position;
  };
  
  //Represents a visible window
  class VisibleWindow : public WindowBase
  {
    public:
      //The title of the window
      const std::string title;
    
      //Creates a new window with the given window title
      VisibleWindow(const std::string &title);
      VisibleWindow(const VisibleWindow &original) = delete; //Explicitly delete the copy constructor since duplicating windows is problematic
      //Does nothing, but needs to call Hide in a derived class (to be overwritten in derived classes). The destructor of this class cannot call Hide as it would thereby invoke pure virtual functions.
      virtual ~VisibleWindow();
      
      //Shows the window if it is not yet shown
      void Show() override;
      //Hides the window
      void Hide() override;
      //Returns true when the window is being shown, and false otherwise
      bool IsShown() const;
      
      //Updates the window after it has been shown (to be overwritten in derived classes)
      virtual void Update(const bool first_update = false) = 0;
      
      //Waits for the specified timeout if the window is shown (0 means infinite). The waiting process can be aborted by a key press in the window. In this case, the pressed key is returned, -1 otherwise (to be overwritten in derived classes).
      virtual int Wait(const int timeout = 0) = 0;
      //Waits for the minimum amount of time (1) and returns the key pressed in the window during this time, if any, or -1 false otherwise.
      virtual int WaitMinimal();
      //Shows the window, optionally call the specified callback function, and wait until a key is pressed or until the specified wait time (0 means infinite) has passed. The key that was pressed to stop the interaction is returned, if any. Hide is called if this is enabled.
      int ShowInteractive(const std::function<void()> &after_show_callback = {}, const int wait_time = 0, const bool hide_after = true);
    protected:
      //False unless the window is shown
      bool shown;
      
      //Creates the window (to be overwritten in derived classes)
      virtual void CreateWindow() = 0;
      //This method is called directly after the window has been created (to be overwritten in derived classes)
      virtual void AfterCreateWindow() = 0;
      //Destroys the window on hiding (to be overwritten in derived classes)
      virtual void DestroyWindow() = 0;
      
      //TODO: These values are specific for Xfce on Debian. If there is an API to get these values directly, use it instead of the hard-coded values
      static constexpr auto outer_border_size = 1;
      static constexpr auto inner_border_size = 4;
      static constexpr auto title_bar_height = 28;
      static constexpr auto tool_bar_height = 39;
      static constexpr auto status_bar_height = 20;
      static constexpr auto desktop_offset_height = 26 + outer_border_size; //The start of the window area below the Xfce panel. Windows cannot be positioned on top of it in its default configuration. Since the panel can be configured and/or moved, this value would need to be changed when the default configuration is not used.
      
      //Adds the size of borders and other elements to a window and returns the total window width and height based on the specified window width and height
      static cv::Size GetTotalSize(const cv::Size &size);
  };
  
  //Represents a window with its contents
  class Window : public VisibleWindow
  {
    public:
      //Creates a new window with the given window title and content as well as an optional size (zeros mean that the size of the content is used). If no content is specified, a dummy image is used.
      Window(const std::string &title, const cv::Mat &content = cv::Mat(1, 1, CV_8UC3, cv::Scalar(0, 0, 0)), const cv::Size &size = cv::Size());
      //Hides the window
      ~Window() override;
      
      //Adds a window control to the window. The name of the control must be unique within the window. If the hidden-window flag is set to true, the control can only be added when the window is not shown.
      void AddControl(WindowControlBase * const control, const bool requires_hidden_window = true);
      //Removes a window control from the window. If the hidden-window flag is set to true, the control can only be removed when the window is not shown.
      void RemoveControl(WindowControlBase * const control, const bool requires_hidden_window = true);
      
      //Returns the width and height of the window (including any borders and controls)
      cv::Size GetSize() const override;
      //Sets the width and the height of the window content (zeros mean that the size of the content is used)
      void SetSize(const cv::Size &size) override;
      //Sets the width and the height of the window content so that the content is scaled by the zoom factor
      void Zoom(const double zoom_factor);
      //Sets the width and the height of the window content so that the content is shown zoomed such that the pixel values become visible
      void ZoomFully();
      
      //Returns the position of the window
      cv::Point GetPosition() const override;
      //Sets the position of the window
      void SetPosition(const cv::Point &position) override;
      
      //Updates the window if it is visible and sets its position and size
      void Update(const bool first_update = false) override;
      
      //Waits for the specified timeout if the window is shown (0 means infinite). The waiting process can be aborted by a key press in the window. In this case, the pressed key is returned, -1 otherwise.
      int Wait(const int timeout = 0) override;
      
      //Specifies whether the window is always shown as enhanced regardless of it having controls requiring this
      void SetAlwaysShowEnhanced(const bool always_show_enhanced = true);
      //Specifies whether the window position is set as if the window were enhanced so that it aligns with other enhanced windows next to it
      void SetPositionLikeEnhanced(const bool position_like_enhanced = true);
      
      //Shows an overlay text for the specified (optional) timeout if the window is shown. If subtle is set to true, the overlay is only shown in the status bar, but not over the image content.
      void ShowOverlayText(const std::string &text, const bool subtle = false, const int timeout = 1000);
      
      //Replaces the window content and redraws the window if it is shown
      void UpdateContent(const cv::Mat &content);
    protected:
      //The content of the window
      cv::Mat content;
      //The controls corresponding to this window in order of their appearance
      std::vector<WindowControlBase*> controls;
      //Specifies whether the window is always shown as an enhanced window
      bool always_show_enhanced;
      //Specifies whether the position is set as if the window were an enhanced window
      bool position_like_enhanced;

      //Returns whether the window uses an enhanced UI since any of its controls requires that
      bool IsEnhanced() const;
      
      //Creates the window and its controls
      void CreateWindow() override;
      //Adds controls to the window which require the window to have been already created
      void AfterCreateWindow() override;
      //Destroys the window on hiding
      void DestroyWindow() override;
      
      //Returns the size of the content (either the actual size when the size is specified as zero, or the specified size otherwise)
      cv::Size GetContentSize() const;
  };
}

#include "winctrl.hpp"
