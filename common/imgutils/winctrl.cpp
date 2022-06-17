//Window controls
// Andreas Unterweger, 2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cassert>

#include <opencv2/highgui.hpp>

#include "winctrl.hpp"

namespace imgutils
{
  WindowControlBase::WindowControlBase(const std::string &name, Window &parent, const bool requires_hidden_window) 
   : name(name), parent(parent), requires_hidden_window(requires_hidden_window)
  {
    parent.AddControl(this, requires_hidden_window);
  }
  
  WindowControlBase::~WindowControlBase()
  {
    parent.RemoveControl(this, requires_hidden_window);
  }
}
