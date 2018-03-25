//Cosine wave generator class (template implementation)
// Andreas Unterweger, 2017
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cmath>

//#include "coswave.hpp"

namespace sndutils
{
  using namespace std;

  template<typename T>
  CosineWaveGenerator<T>::CosineWaveGenerator(const double frequency, const double amplitude, const bool absolute_amplitude, const double initial_phase, const unsigned int sampling_rate)
   : SineWaveGenerator<T>(frequency, amplitude, absolute_amplitude, initial_phase + M_PI_2, sampling_rate) { } //Phase shift of pi/2 between cosine and sine
   
  template<typename T>
  double CosineWaveGenerator<T>::GetInitialPhase() const
  {
    return SineWaveGenerator<T>::GetInitialPhase() - M_PI_2; //Phase shift of pi/2 between cosine and sine
  }
}
