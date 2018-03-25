//Wave form mixer class (header)
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <array>

#include "wavegen.hpp"

namespace sndutils
{
  using namespace std;
  
  //Defines a mixer of M wave forms
  template<typename T, size_t M>
  class WaveFormMixer : public WaveFormGenerator<T>
  {  
    public:
      //Constructs a new instance of WaveFormGenerator with the given channel parameters
      WaveFormMixer(array<WaveFormGenerator<T>*, M> generators, const double mixing_factor = 1.0 / M, const unsigned int sampling_rate = 48000);
      
      //Replaces the component at the specified index with the given component
      void SetGenerator(const size_t generator_index, WaveFormGenerator<T> * const generator);
      
      //Produces the next sample of the added weighted sum of all generators specified during construction. The weights are equal to the mixing factor.
      T GetNextSample();
      
      //Produces the added weighted sum of the first N representative samples of each generator
      void GetRepresentativeSamples(const size_t N, T values[]) const;
      
    private:
      array<WaveFormGenerator<T>*, M> generators;
      double mixing_factor;
      
      double MixValue(const double value) const;
      T ClipValue(const double value) const;
      void VerifyGenerator(const WaveFormGenerator<T> * const generator) const;
  };
}

#include "mixer.impl.hpp"
