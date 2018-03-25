//Mathematical helper functions
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cmath>

#include "math.hpp"

namespace comutils
{  
  double GetLevelFromValue(const double level, const double reference_value)
  {
    return 20 * log10(level / reference_value);
  }
  
  double GetValueFromLevel(const double level, const double reference_value)
  {
    return reference_value * pow(10, level / 20);
  }
}
