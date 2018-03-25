//String formatting functions (header)
// Andreas Unterweger, 2016-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

namespace comutils
{
  using namespace std;
  
  //Formats a given floating-point value with the specified number of decimal places after the decimal point
  string FormatValue(const double value, const unsigned int decimals = 2);
  //Formats a decibel value
  string FormatLevel(const double value);
  //Formats a value representing a size in bytes, e.g., 2048 = 2.0 kiB
  string FormatByte(const unsigned int value);
}
