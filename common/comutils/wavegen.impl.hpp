//Wave form generator interface (template implementation)
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cassert>

//#include "wavegen.hpp"

namespace comutils
{
  using namespace std;

  template<typename T>
  WaveFormGenerator<T>::WaveFormGenerator(const unsigned int sampling_rate) : sampling_rate(sampling_rate)
  {
    assert(sampling_rate > 0);
  }

  template<typename T>
  unsigned int WaveFormGenerator<T>::GetSamplingRate() const
  {
    return sampling_rate;
  }
}
