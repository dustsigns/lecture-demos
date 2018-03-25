//Mathematical helper functions (template implementation)
// Andreas Unterweger, 2016-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cmath>

//#include "math.hpp"

namespace comutils
{  
  template<typename T>
  T sqr(const T value)
  {
    return value * value;
  }
  
  constexpr double DegreesToRadians(const double degrees)
  {
    return M_PI * degrees / 180;
  }

  constexpr double RadiansToDegrees(const double radians)
  {
    return 180 * radians / M_PI;
  }
  
  constexpr double GetDCTCoefficientScalingFactor(const unsigned int block_size, const unsigned int i)
  {
    return sqrt((i == 0 ? 1.0 : 2.0) / block_size); //DC coefficient has an additional factor of sqrt(2)
  }

  constexpr double GetIDCTCoefficientScalingFactor(const unsigned int block_size, const unsigned int i)
  {
    return 1 / GetDCTCoefficientScalingFactor(block_size, i);
  }

  constexpr double Get2DDCTCoefficientScalingFactor(const unsigned int block_size, const unsigned int i, const unsigned int j)
  {
    return GetDCTCoefficientScalingFactor(block_size, i) * GetDCTCoefficientScalingFactor(block_size, j);
  }

  constexpr double Get2DIDCTCoefficientScalingFactor(const unsigned int block_size, const unsigned int i, const unsigned int j)
  {
    return GetIDCTCoefficientScalingFactor(block_size, i) * GetIDCTCoefficientScalingFactor(block_size, j);
  }
}
