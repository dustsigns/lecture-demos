//Sine wave generator class (template implementation)
// Andreas Unterweger, 2017-2019
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cassert>
#include <cmath>

//#include "sinewave.hpp"

namespace sndutils
{
  using namespace std;

  template<typename T>
  SineWaveGenerator<T>::SineWaveGenerator(const double frequency, const double amplitude, const bool absolute_amplitude, const double initial_phase, const unsigned int sampling_rate)
   : WaveFormGenerator<T>(sampling_rate),
     absolute_amplitude(absolute_amplitude),
     phase(0)
  {
    SetAmplitude(amplitude);
    SetFrequency(frequency);
    SetPhaseShift(initial_phase);
  }
  
  template<typename T>
  double SineWaveGenerator<T>::GetAmplitude() const
  {
    return this->amplitude;
  }
  
  template<typename T>
  double SineWaveGenerator<T>::GetFrequency() const
  {
    return this->frequency;
  }
  
  template<typename T>
  double SineWaveGenerator<T>::GetInitialPhase() const
  {
    if (this->frequency == 0.0)
      return 0;
    else
      return (2 * M_PI * this->phase_shift) / this->units_per_period;
  }
  
  template<typename T>
  void SineWaveGenerator<T>::SetAmplitude(const double amplitude)
  {
    if (!this->absolute_amplitude)
      assert(amplitude >= 0.0 && amplitude <= 1.0);
    this->amplitude = amplitude;
  }
  
  template<typename T>
  void SineWaveGenerator<T>::SetFrequency(const double frequency)
  {
    assert(frequency >= 0.0/* && frequency < this->sampling_rate / 2*/); //Uncomment if subsampling is undesired
    this->frequency = frequency;
    this->units_per_period = this->sampling_rate / this->frequency;
  }
  
  template<typename T>
  T SineWaveGenerator<T>::GetSample(const double selected_phase) const
  {
    assert(selected_phase >= 0.0); //Comment this if a purely positive phase is desired
    const double relative_amplitude = this->amplitude * (this->absolute_amplitude ? 1.0 : WaveFormGenerator<T>::max_amplitude);
    const double current_amplitude = relative_amplitude * (this->frequency == 0 ? 1.0 : sin(2 * M_PI * this->frequency * (selected_phase + this->phase_shift) / this->sampling_rate)); //DC is multiplication with 1.0
    const T selected_value = static_cast<T>(current_amplitude);
    return selected_value;
  }

  template<typename T>
  T SineWaveGenerator<T>::GetNextSample()
  {
    const T current_value = GetSample(this->phase);
    if (this->frequency != 0.0)
      this->phase = fmod(this->phase + 1, this->units_per_period);
    return current_value;
  }
  
  template<typename T>
  void SineWaveGenerator<T>::GetRepresentativeSamples(const size_t N, T values[]) const
  {
    for (size_t i = 0; i < N; i++)
      values[i] = GetSample(i);
  }
  
  template<typename T>
  void SineWaveGenerator<T>::SetPhaseShift(const double initial_phase)
  {
    if (this->frequency != 0.0)
    {
      const auto simple_initial_phase = fmod(initial_phase, 2 * M_PI);
      this->phase_shift = this->units_per_period * simple_initial_phase / (2 * M_PI);
    }
  }
}
