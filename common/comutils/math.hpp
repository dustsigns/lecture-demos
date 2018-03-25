//Mathematical helper functions (header)
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

namespace comutils
{  
  //Squares the given value
  template<typename T>
  T sqr(const T value);
  
  //Converts an angle from degrees to radians
  constexpr double DegreesToRadians(const double degrees);
  //Converts an angle from radians to degrees
  constexpr double RadiansToDegrees(const double radians);
  
  //Converts a value into a level [dB] relative to the specified reference value
  double GetLevelFromValue(const double value, const double reference_value);
  //Converts a level [dB] into a value relative to the specified reference value
  double GetValueFromLevel(const double level, const double refrence_value);
  
  //Returns the factor by which the ith DCT coefficient is scaled after transform
  constexpr double GetDCTCoefficientScalingFactor(const unsigned int block_size, const unsigned int i);
  //Returns the factor by which the ith DCT coefficient is scaled after inverse transform
  constexpr double GetDCTCoefficientScalingFactor(const unsigned int block_size, const unsigned int i);
  //Returns the factor by which the 2-D-DCT coefficient with indices (i, j) is scaled after transform
  constexpr double Get2DDCTCoefficientScalingFactor(const unsigned int block_size, const unsigned int i, const unsigned int j);
  //Returns the factor by which the 2-D-DCT coefficient with indices (i, j) is scaled after inverse transform
  constexpr double Get2DIDCTCoefficientScalingFactor(const unsigned int block_size, const unsigned int i, const unsigned int j);
}

#include "math.impl.hpp"
