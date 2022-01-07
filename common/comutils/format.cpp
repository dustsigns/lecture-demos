//String formatting functions
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <sstream>
#include <iomanip>

#include "format.hpp"

namespace comutils
{
  std::string FormatValue(const double value, const unsigned int decimals)
  {
    std::ostringstream stream;
    stream << std::fixed << std::showpoint << std::setprecision(decimals) << value;
    return stream.str();
  }

  std::string FormatLevel(const double value)
  {
    return FormatValue(value) + " dB";
  }

  std::string FormatByte(const unsigned int value)
  {
    constexpr const char prefixes[] = {'k', 'M', 'G', 'T', 'E'};
    std::ostringstream stream;
    if (value < 1024)
      stream << value << " ";
    else
    {
      int prefix_idx = -1;
      double converted_value = value;
      do
      {
        prefix_idx++;
      } while ((converted_value /= 1024) >= 1024);
      stream << std::fixed << FormatValue(converted_value) << " " << prefixes[prefix_idx] << 'i';
    }
    stream << "B";
    return stream.str();
  }
}
