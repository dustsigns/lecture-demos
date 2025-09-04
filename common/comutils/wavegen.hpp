//Wave form generator interface (header)
// Andreas Unterweger, 2017-2025
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <cstddef>

namespace comutils
{
  //Defines an abstract generator of wave forms
  template<typename T>
  class WaveFormGenerator
  {
    public:
      //Constructs a new instance of WaveFormGenerator with the given channel parameters
      WaveFormGenerator(const unsigned int sampling_rate = 48000);
      virtual ~WaveFormGenerator() = default;
      
      //Produces the next sample of the wave form. To be implemented in child classes.
      virtual T GetNextSample() = 0;
      
      //Produces N representative samples of the wave form. To be implemented in child classes.
      virtual void GetRepresentativeSamples(const size_t N, T values[]) const = 0;
      
      //Returns the sampling rate used during construction
      unsigned int GetSamplingRate() const;
      
    protected:
      static constexpr T min_amplitude = std::numeric_limits<T>::lowest();
      static constexpr T max_amplitude = std::numeric_limits<T>::max();
      
      unsigned int sampling_rate;
  };
}

#include "wavegen.impl.hpp"
