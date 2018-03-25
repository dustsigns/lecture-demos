//Commonly used functions (template implementation)
// Andreas Unterweger, 2016-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

//#include "common.hpp"

namespace comutils
{
  using namespace std;
  
  template<typename T, size_t N>
  constexpr size_t arraysize(const T (&)[N])
  {
    return N;
  }
}
