//Commonly used functions (header)
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <cstddef>

namespace comutils
{
  //Determines the number of elements of the given array
  template<typename T, size_t N>
  constexpr size_t arraysize(const T (&)[N]);
}

#include "common.impl.hpp"
