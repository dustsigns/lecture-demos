//Window controls (template implementation)
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cassert>

#include <opencv2/highgui.hpp>

//#include "winctrl.hpp"

namespace imgutils
{  
  template<size_t N, typename... Ts>
  WindowControl<N, Ts...>::WindowControl(const std::string &name, Window &parent, const bool requires_hidden_window, const std::array<ControlCallback*, N> &callbacks, Ts const ...tags)
   : WindowControlBase(name, parent, requires_hidden_window),
     callbacks(callbacks), tags(tags...) { }
  
  template<size_t N, typename... Ts>
  void WindowControl<N, Ts...>::TriggerCallback(WindowControl<N, Ts...> * const control, const size_t index)
  {
    assert(control->callbacks.size() > index);
    const auto &callback = control->callbacks[index];
    if (control->parent.IsShown() && callback) //Only call callback when window is shown
      std::apply(callback, control->tags); //Unpack tuple (tags) as parameters
  }
  
  template<typename... Ts>
  SimpleWindowControl<Ts...>::SimpleWindowControl(const std::string &name, Window &parent, const bool requires_hidden_window, ControlCallback * const callback, Ts const ...tags)
   : WindowControl<1, Ts...>(name, parent, requires_hidden_window, {{callback}}, tags...) { }
  
  template<typename... Ts>
  MouseEvent<Ts...>::MouseEvent(Window &parent, ControlCallback * const callback, Ts const ...tags) : 
    SimpleWindowControl<int, int, int, Ts...>("Mouse event", parent, false, callback, -1, -1, -1, tags...) { } //Dummy name, no hidden window required; dummy values for mouse-event related parts of the tags
  
  template<typename... Ts>
  void MouseEvent<Ts...>::Render()
  {
    cv::setMouseCallback(this->parent.title,
                         [](const int event, const int x, const int y, const int, void * const userdata)
                           {
                             const auto control = static_cast<MouseEvent*>(userdata);
                             std::get<0>(control->tags) = event;
                             std::get<1>(control->tags) = x;
                             std::get<2>(control->tags) = y;
                             MouseEvent<Ts...>::TriggerCallback(control);
                           }, this);
  }
  
  template<typename... Ts>
  bool MouseEvent<Ts...>::RequiresEnhancedWindow() const
  {
    return false;
  }
   
  template<typename... Ts>
  Button<Ts...>::Button(const std::string &name, Window &parent, ControlCallback * const callback, Ts const ...tags) : 
    SimpleWindowControl<Ts...>(name, parent, true, callback, tags...) { } //Requires hidden window
  
  template<typename... Ts>
  void Button<Ts...>::Render()
  {
    cv::createButton(this->name, [](const int, void * const userdata)
                                   {
                                     const auto control = static_cast<Button*>(userdata);
                                     Button<Ts...>::TriggerCallback(control);
                                   }, this, cv::QT_PUSH_BUTTON);
  }
  
  template<typename... Ts>
  bool Button<Ts...>::RequiresEnhancedWindow() const
  {
    return true;
  }
  
  template<typename... Ts>
  TrackBar<Ts...>::TrackBar(const std::string &name, Window &parent, const int max_value, const int min_value, const int default_value, ControlCallback * const callback, Ts const ...tags) :
    SimpleWindowControl<Ts...>(name, parent, true, callback, tags...), //Requires hidden window
    max_value(max_value), min_value(min_value), default_value(default_value)
  {
    assert(min_value <= max_value);
    assert(default_value >= min_value);
    assert(default_value <= max_value);
  }
  
  template<typename... Ts>
  int TrackBar<Ts...>::GetValue() const
  {
    if (this->parent.IsShown())
      return cv::getTrackbarPos(this->name, this->parent.title);
    else
      return default_value;
  }
  
  template<typename... Ts>
  void TrackBar<Ts...>::SetValue(const int value)
  {
    if (this->parent.IsShown())
      cv::setTrackbarPos(this->name, this->parent.title, value);
    else
      default_value = value;
  }
  
  template<typename... Ts>
  void TrackBar<Ts...>::Render()
  {
    cv::createTrackbar(this->name, this->parent.title, nullptr, this->max_value,
                       [](const int, void * const userdata)
                         {
                           const auto control = static_cast<TrackBar*>(userdata);
                           TrackBar<Ts...>::TriggerCallback(control);
                         }, this);
    cv::setTrackbarMin(this->name, this->parent.title, this->min_value);
    cv::setTrackbarMax(this->name, this->parent.title, this->max_value);
    cv::setTrackbarPos(this->name, this->parent.title, this->default_value);
  }
  
  template<typename... Ts>
  bool TrackBar<Ts...>::RequiresEnhancedWindow() const
  {
    return false;
  }

  template<typename... Ts>
  CheckableControl<Ts...>::CheckableControl(const std::string &name, Window &parent, const bool default_checked, ControlCallback * const checked_callback, ControlCallback * const unchecked_callback, Ts const ...tags) : 
    WindowControl<2, Ts...>(name, parent, true, {checked_callback, unchecked_callback}, tags...),
    default_checked(default_checked) { }
  
  template<typename... Ts>
  bool CheckableControl<Ts...>::IsDefaultChecked() const
  {
    return default_checked;
  }
  
  template<typename... Ts>
  void CheckableControl<Ts...>::SetDefaultChecked(const bool checked)
  {
    default_checked = checked;
  }
  
  template<typename... Ts>
  void CheckableControl<Ts...>::Render()
  {
    const auto control_type = GetControlType();
    cv::createButton(this->name, [](const int state, void * const userdata)
                                   {
                                     const auto control = static_cast<CheckableControl*>(userdata);
                                     size_t callback_index = state ? 0 : 1;
                                     CheckableControl<Ts...>::TriggerCallback(control, callback_index);
                                   }, this, control_type, default_checked);
  }
  
  template<typename... Ts>
  bool CheckableControl<Ts...>::RequiresEnhancedWindow() const
  {
    return true;
  }

  template<typename... Ts>
  RadioButton<Ts...>::RadioButton(const std::string &name, Window &parent, const bool default_checked, ControlCallback * const checked_callback, ControlCallback * const unchecked_callback, Ts const ...tags) : 
    CheckableControl<Ts...>(name, parent, default_checked, checked_callback, unchecked_callback, tags...) { }
  
  template<typename... Ts>
  int RadioButton<Ts...>::GetControlType() const
  {
    return cv::QT_RADIOBOX;
  }

  template<typename... Ts>
  CheckBox<Ts...>::CheckBox(const std::string &name, Window &parent, const bool default_checked, ControlCallback * const checked_callback, ControlCallback * const unchecked_callback, Ts const ...tags) : 
    CheckableControl<Ts...>(name, parent, default_checked, checked_callback, unchecked_callback, tags...) { }
  
  template<typename... Ts>
  int CheckBox<Ts...>::GetControlType() const
  {
    return cv::QT_CHECKBOX;
  }
}
