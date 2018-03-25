//Helper functions for calculations on images (template implementations)
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

//#include "imgmath.hpp"

namespace imgutils
{
  using namespace std;
  
  constexpr double LevelShift(const double value)
  {
    return value - 128;
  }

  constexpr double ReverseLevelShift(const double value)
  {
    return value + 128;
  }
}
