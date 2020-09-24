//Sine wave generator class (header)
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include "wavegen.hpp"

namespace comutils
{
  using namespace std;
  
  //Defines a generator for sinusodial wave forms
  template<typename T>
  class SineWaveGenerator : public WaveFormGenerator<T>
  {  
    public:
      //Constructs a new instance of SineWaveGenerator with the given channel parameters. If absolute_amplitude is true, amplitude is used as is. If it is false, amplitude is used relative to the value range of T.
      SineWaveGenerator(const double frequency, const double amplitude = 1.0, const bool absolute_amplitude = false, const double initial_phase = 0, const unsigned int sampling_rate = 48000);
      
      //Returns the amplitude of the sine wave
      double GetAmplitude() const;
      //Returns the frequency of the sine wave
      double GetFrequency() const;
      //Returns the initial phase of the sine wave
      virtual double GetInitialPhase() const;
      
      //Sets the amplitude of the sine wave
      void SetAmplitude(const double amplitude);
      //Sets the frequency of the sine wave
      void SetFrequency(const double frequency);
      
      //Produces the next sample of the sine wave
      T GetNextSample();
      
      //Produces first N samples of the sine wave
      void GetRepresentativeSamples(const size_t N, T values[]) const;
      
    private:
      double amplitude;
      double frequency;
      bool absolute_amplitude;
      double units_per_period;
      double phase; //Current phase
      double phase_shift; //Initial phase shift
      
      T GetSample(const double selected_phase) const;
      
      void SetPhaseShift(const double phase);
  };
}

#include "sinewave.impl.hpp"
