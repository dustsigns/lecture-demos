//Cosine wave generator class (header)
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include "sinewave.hpp"

namespace comutils
{
  using namespace std;
  
  //Defines a generator for cosinusodial wave forms
  template<typename T>
  class CosineWaveGenerator : public SineWaveGenerator<T>
  {  
    public:
      //Constructs a new instance of CosineWaveGenerator with the given channel parameters. If absolute_amplitude is true, amplitude is used as is. If it is false, amplitude is used relative to the value range of T.
      CosineWaveGenerator(const double frequency, const double amplitude = 1.0, const bool absolute_amplitude = false, const double initial_phase = 0, const unsigned int sampling_rate = 48000);
      
      //Returns the initial phase of the cosine wave
      virtual double GetInitialPhase() const;
  };
}

#include "coswave.impl.hpp"
