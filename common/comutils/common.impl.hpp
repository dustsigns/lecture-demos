//Commonly used functions (template implementation)
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

//#include "common.hpp"

namespace comutils
{
  template<typename T, size_t N>
  constexpr size_t arraysize(const T (&)[N])
  {
    return N;
  }
}
