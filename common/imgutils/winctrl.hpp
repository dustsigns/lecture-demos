//Window controls (header)
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <array>
#include <tuple>
#include <map>

#include <opencv2/core.hpp>

#include "window.hpp"

namespace imgutils
{
  //Represents the base class for a control element in a window
  class WindowControlBase
  {
    public:
      //The name of the window control
      const std::string name;
      //The parent window the window control is associated with
      Window &parent;
      //True if this particular control requires the window to be hidden when it is added or rendered
      const bool requires_hidden_window;
      
      //Constructs a new window control with the specified name and parent window. It is automatically added to the parent window
      WindowControlBase(const std::string &name, Window &parent, const bool requires_hidden_window = true);
      //Explicitly delete the copy constructor since duplicating controls tied to windows is problematic
      WindowControlBase(const WindowControlBase &original) = delete;
      ~WindowControlBase();
      
      //Renders the control into the parent window (to be overwritten in derived classes). This method is called by the parent window
      virtual void Render() = 0;
      
      //Returns true if this particular control requires an enhanced window to be rendered (to be overwritten in derived classes)
      virtual bool RequiresEnhancedWindow() const = 0;
  };
  
  //Represents a control element in a window with N callbacks and optional tag types Ts
  template<size_t N, typename... Ts>
  class WindowControl : public WindowControlBase
  {
    public:
      //Defines a callback function when an event has been triggered for the control
      using ControlCallback = void(Ts const ...tags);
    
      //Constructs a new window control with the specified name, parent window and optional callbacks and tags. It is automatically added to the parent window
      WindowControl(const std::string &name, Window &parent, const bool requires_hidden_window = true, const std::array<ControlCallback*, N> &callbacks = {{}}, Ts const ...tags);
    protected:
      //Callbacks supported by the window control
      const std::array<ControlCallback*, N> callbacks;
      //Optional tags
      std::tuple<Ts...> tags;
      
      //Triggers the callback with the specified index
      static void TriggerCallback(WindowControl<N, Ts...> * const control, const size_t index = 0);
  };

  //Represents a simple control element with a single callback
  template<typename... Ts>
  class SimpleWindowControl : public WindowControl<1, Ts...>
  {
    public:
      using typename WindowControl<1, Ts...>::ControlCallback;
      //Constructs a new window control with the specified name, parent window and optional callback and tags. It is automatically added to the parent window
      SimpleWindowControl(const std::string &name, Window &parent, const bool requires_hidden_window = true, ControlCallback * const callback = nullptr, Ts const ...tags);
    protected:
      using WindowControl<1, Ts...>::TriggerCallback;
  };
  
  //Represents a control which handles mouse events. The callback has three additional parameters (the event, the X and the Y coordinate, all integers)
  template<typename... Ts>
  class MouseEvent : public SimpleWindowControl<int, int, int, Ts...>
  {
    public:
      using typename SimpleWindowControl<int, int, int, Ts...>::ControlCallback;
      //Constructs a new mouse event handler for the specified parent window with the specified optional tags. It is automatically added to the parent window
      MouseEvent(Window &parent, ControlCallback * const callback, Ts const ...tags);
      
      //Registers the mouse event handler. This method is called by the parent window.
      void Render();
      
      //Returns true if this particular control requires an enhanced window to be rendered
      bool RequiresEnhancedWindow() const override;
    protected:
      using SimpleWindowControl<int, int, int, Ts...>::TriggerCallback;
  };
  
  //Represents a button control
  template<typename... Ts>
  class Button : public SimpleWindowControl<Ts...>
  {
    public:
      using typename SimpleWindowControl<Ts...>::ControlCallback;
      //Constructs a new instance of a button in the given parent window with the specified optional callback and tags
      Button(const std::string &name, Window &parent, ControlCallback * const callback = nullptr, Ts const ...tags);
      
      //Renders the button into the parent window. This method is called by the parent window.
      void Render() override;
      
      //Returns true if this particular control requires an enhanced window to be rendered
      bool RequiresEnhancedWindow() const override;
    protected:
      using SimpleWindowControl<Ts...>::TriggerCallback;
  };
  
  //Represents a trackbar control
  template<typename... Ts>
  class TrackBar : public SimpleWindowControl<Ts...>
  {
    public:
      using typename SimpleWindowControl<Ts...>::ControlCallback;
      //Constructs a new instance of a track bar in the given parent window with the specified minimum, maximum, default value, callback and optional tags
      TrackBar(const std::string &name, Window &parent, const int max_value, const int min_value, const int default_value, ControlCallback * const callback = nullptr, Ts const ...tags);
      
      //Returns the current trackbar value, or the default value if the window is not shown
      int GetValue() const;
      //Sets the trackbar value. If the window is not shown, the default value is changed.
      void SetValue(const int value);
      
      //Renders the trackbar into the parent window. This method is called by the parent window.
      void Render() override;
      
      //Returns true if this particular control requires an enhanced window to be rendered
      bool RequiresEnhancedWindow() const override;
    protected:
      using SimpleWindowControl<Ts...>::TriggerCallback;
      //The trackbar maximum value
      int max_value;
      //The minimum trackbar value
      int min_value;
      //The initial trackbar value
      int default_value;
  };
  
  //Represents a checkable control
  template<typename... Ts>
  class CheckableControl : public WindowControl<2, Ts...>
  {
    public:
      using typename WindowControl<2, Ts...>::ControlCallback;
      //Constructs a new instance of a checkable control in the given parent window with the specified default checked state and optional callbacks and tags
      CheckableControl(const std::string &name, Window &parent, const bool default_checked = false, ControlCallback * const checked_callback = nullptr, ControlCallback * const unchecked_callback = nullptr, Ts const ...tags);
      
      //Returns whether the checkable control is checked by default (may differ if the window is shown)
      bool IsDefaultChecked() const;
      //Sets the default checked state of the checkable control. This does not change the state once the window is shown.
      void SetDefaultChecked(const bool checked);
      
      //Renders the checkable control into the parent window. This method is called by the parent window.
      void Render() override;
      
      //Returns true if this particular control requires an enhanced window to be rendered
      bool RequiresEnhancedWindow() const override;
    protected:
      using WindowControl<2, Ts...>::TriggerCallback;
      //Returns the type of control to be rendered (to be overwritten in child classes)
      virtual int GetControlType() const = 0;
    
      //Whether the checkable control is checked by default
      bool default_checked;
  };
  
  //Represents a radio button control
  template<typename... Ts>
  class RadioButton : public CheckableControl<Ts...>
  {
    public:
      using typename CheckableControl<Ts...>::ControlCallback;
      //Constructs a new instance of a radio button in the given parent window with the specified default checked state and optional callbacks and tags
      RadioButton(const std::string &name, Window &parent, const bool default_checked = false, ControlCallback * const checked_callback = nullptr, ControlCallback * const unchecked_callback = nullptr, Ts const ...tags);
    protected:
      using CheckableControl<Ts...>::TriggerCallback;
      //Returns the type of control to be rendered (radio button)
      int GetControlType() const override;
  };

  //Represents a check box control
  template<typename... Ts>
  class CheckBox : public CheckableControl<Ts...>
  {
    public:
      using typename CheckableControl<Ts...>::ControlCallback;
      //Constructs a new instance of a check box in the given parent window with the specified default checked state and optional callbacks and tags
      CheckBox(const std::string &name, Window &parent, const bool default_checked = false, ControlCallback * const checked_callback = nullptr, ControlCallback * const unchecked_callback = nullptr, Ts const ...tags);
    protected:
      using CheckableControl<Ts...>::TriggerCallback;
      //Returns the type of control to be rendered (check box)
      int GetControlType() const override;
  };
}

#include "winctrl.impl.hpp"
