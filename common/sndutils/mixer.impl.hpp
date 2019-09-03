//Wave form mixer class (template implementation)
// Andreas Unterweger, 2017
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cassert>
#include <vector>

//#include "mixer.hpp"

namespace sndutils
{
  using namespace std;

  template<typename T, size_t M>
  WaveFormMixer<T, M>::WaveFormMixer(const array<WaveFormGenerator<T>*, M> &generators, const double mixing_factor, const unsigned int sampling_rate)
   : WaveFormGenerator<T>(sampling_rate),
     generators(generators), mixing_factor(mixing_factor)
  {
    for (const auto * const generator : generators)
      VerifyGenerator(generator);
    assert(mixing_factor >= 0.0 && mixing_factor <= 1.0);
  }
  
  template<typename T, size_t M>
  void WaveFormMixer<T, M>::SetGenerator(const size_t generator_index, WaveFormGenerator<T> * const generator)
  {
    assert(generator_index < M);
    this->generators[generator_index] = generator;
    VerifyGenerator(generator);
  }
  
  template<typename T, size_t M>
  double WaveFormMixer<T, M>::MixValue(const double value) const
  {
    return mixing_factor * value;
  }
  
  template<typename T, size_t M>
  T WaveFormMixer<T, M>::ClipValue(const double value) const
  {
    double clipped_value = value;
    if (clipped_value < WaveFormGenerator<T>::min_amplitude)
      clipped_value = static_cast<double>(WaveFormGenerator<T>::min_amplitude);
    if (clipped_value > WaveFormGenerator<T>::max_amplitude)
      clipped_value = static_cast<double>(WaveFormGenerator<T>::max_amplitude);
    return static_cast<T>(clipped_value);
  }

  template<typename T, size_t M>
  T WaveFormMixer<T, M>::GetNextSample()
  {
    double sample_value = 0.0;
    for (auto * const generator : generators)
    {
      if (generator)
        sample_value += MixValue(generator->GetNextSample());
    }
    return ClipValue(sample_value);
  }
  
  template<typename T, size_t M>
  void WaveFormMixer<T, M>::GetRepresentativeSamples(const size_t N, T values[]) const
  {
    vector<double> sample_values(N, 0.0);
    for (auto * const generator : generators)
    {
      if (generator)
      {
        vector<T> representative_samples(N);
        generator->GetRepresentativeSamples(N, representative_samples.data());
        for (size_t i = 0; i < N; i++)
          sample_values[i] +=  MixValue(representative_samples[i]);
      }
    }
    transform(sample_values.begin(), sample_values.end(), values, [this](const double value)
                                                                        {
                                                                          return ClipValue(value);
                                                                        });
  }
  
  template<typename T, size_t M>
  void WaveFormMixer<T, M>::VerifyGenerator(const WaveFormGenerator<T> * const generator) const
  {
    if (generator)
      assert(generator->GetSamplingRate() == this->sampling_rate);
  }
}
