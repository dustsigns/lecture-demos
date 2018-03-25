//String formatting functions
// Andreas Unterweger, 2016-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <sstream>
#include <iomanip>

#include "format.hpp"

namespace comutils
{
  using namespace std;

  string FormatValue(const double value, const unsigned int decimals)
  {
    ostringstream stream;
    stream << fixed << showpoint << setprecision(decimals) << value;
    return stream.str();
  }

  string FormatLevel(const double value)
  {
    return FormatValue(value) + " dB";
  }

  string FormatByte(const unsigned int value)
  {
    constexpr const char prefixes[] = {'k', 'M', 'G', 'T', 'E'};
    ostringstream stream;
    if (value < 1024)
      stream << to_string(value) << " ";
    else
    {
      int prefix_idx = -1;
      double converted_value = value;
      do
      {
        prefix_idx++;
      } while ((converted_value /= 1024) >= 1024);
      stream << fixed << FormatValue(converted_value) << " " << prefixes[prefix_idx] << 'i';
    }
    stream << "B";
    return stream.str();
  }
}
