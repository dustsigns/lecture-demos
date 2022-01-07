//Audio playback helper class (template implementation)
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cassert>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <type_traits>

#include "common.hpp"

//#include "player.hpp"

namespace sndutils
{
  template<typename T>
  class WaveFormConverter
  {
    public:
      WaveFormConverter(comutils::WaveFormGenerator<T> &generator, const size_t number_of_channels)
       : generator(generator), number_of_channels(number_of_channels),
         unit_size(sample_size * number_of_channels),
         buffer_size(units_per_buffer * unit_size)
      {
        assert(number_of_channels > 0);
        
        buffer = new unsigned char[buffer_size];
      }
      
      WaveFormConverter(const WaveFormConverter<T>&) = delete; //Disallow copying because buffer pointer would be duplicated and subsequently double-freed
      
      ~WaveFormConverter()
      {
        delete[] buffer;
      }
      
      size_t GetNextSampleBuffer(unsigned char * &buffer)
      {
        buffer = this->buffer;
        static_assert(sizeof(unsigned char) == 1, "char must be one byte in size");
        for (size_t unit = 0; unit < units_per_buffer; unit++)
        {
          static_assert(std::is_integral<T>(), "T (WaveFormGenerator sample type) must be an integral type");
          using U = typename std::conditional<std::is_signed<T>::value, long, unsigned long>::type; //Use long for signed chars, ints etc. and unsigned long for the rest
          static_assert(sizeof(T) <= sizeof(U), "T (WaveFormGenerator sample type) cannot be larger than (unsigned) long"); //Since (unsigned) long is used for shifting below, the type's size has to be smaller than that of an (unsigned) long
          const auto current_value = static_cast<U>(generator.GetNextSample());
          for (size_t channel = 0; channel < number_of_channels; channel++) //Fill all channels with the same samples
          {
            for (size_t sample_byte = 0; sample_byte < sample_size; sample_byte++)
            {
              buffer[unit * unit_size + channel * sample_size + sample_byte] = (current_value >> (8 * sample_byte)) & 0xFF; //Extract (sample_byte)th byte in little-endian byte order
            }
          }
        }
        return buffer_size;
      }
      
      //Sample size in bytes
      static constexpr size_t sample_size = sizeof(T);
    
    private:
      static constexpr size_t units_per_buffer = 1000; //Each buffer will be filled with 1000 samples for each channel
      
      comutils::WaveFormGenerator<T> &generator;
      const size_t number_of_channels;
      const size_t unit_size;
      const size_t buffer_size;
      unsigned char *buffer;
  };

  template<typename T>
  AudioPlayer<T>::AudioPlayer(const unsigned int sampling_rate, const size_t number_of_channels) : playing(false), paused(false)
  {
    assert(sampling_rate > 0);
    
    constexpr size_t sample_bits = 8 * WaveFormConverter<T>::sample_size;
    static_assert(sample_bits == 8 || sample_bits == 16 || sample_bits == 32, "Only 8-bit, 16-bit and 32-bit types are supported"); //TODO: Support 24 bit types?
    sample_format.bits = sample_bits;
    sample_format.channels = static_cast<int>(number_of_channels);
    sample_format.rate = static_cast<int>(sampling_rate);
    sample_format.byte_format = AO_FMT_LITTLE; //Use little endian
    sample_format.matrix = nullptr;
    
    ao_initialize();
    const int driver_id = ao_default_driver_id();
    if (!(playback_device = ao_open_live(driver_id, &sample_format, nullptr)))
      throw std::runtime_error("Could not open playback device");
  }

  template<typename T>
  void AudioPlayer<T>::Play(comutils::WaveFormGenerator<T> &generator)
  {
    if (playing)
      throw std::runtime_error("Already playing. Stop playback first.");
    playing = true;
    
    const size_t this_number_of_channels = this->sample_format.channels;
    const auto &this_playing = this->playing;
    const auto &this_paused = this->paused;
    ao_device * const &this_playback_device = this->playback_device;
    worker = std::thread([&generator, this_number_of_channels, &this_playing, &this_paused, &this_playback_device]
                         ()
                         {
                           WaveFormConverter<T> converter(generator, this_number_of_channels);
                           while (this_playing)
                           {
                             if (this_paused) //Allow other threads to work while waiting for playback to resume
                               std::this_thread::yield();
                             else
                             {
                               unsigned char *buffer;
                               size_t buffer_size = converter.GetNextSampleBuffer(buffer);
                               static_assert(sizeof(char) == 1 && sizeof(char) == sizeof(unsigned char), "Both, char and unsigned char, must be one byte in size");
                               if (!ao_play(this_playback_device, reinterpret_cast<char*>(buffer), buffer_size)) //TODO: Double-buffering to avoid pauses
                                 throw std::runtime_error("Playback error. Don't use this instance again.");
                             }
                           }
                         });
  }

  template<typename T>
  void AudioPlayer<T>::Stop()
  {
    if (!playing)
      return;
    playing = false;
    worker.join();
  }

  template<typename T>
  void AudioPlayer<T>::Pause()
  {
    if (!playing)
      throw std::runtime_error("Not playing. Start playback first.");
    paused = true;
  }

  template<typename T>
  void AudioPlayer<T>::Resume()
  {
    if (!playing)
      throw std::runtime_error("Not playing. Start playback first.");
    paused = false;
  }

  template<typename T>
  bool AudioPlayer<T>::IsPlaying() const
  {
    return playing;
  }

  template<typename T>
  bool AudioPlayer<T>::IsPlayingBack() const
  {
    return playing && !paused;
  }

  template<typename T>
  AudioPlayer<T>::~AudioPlayer()
  {
    Stop();
    ao_close(playback_device);
    ao_shutdown();
  }
}
