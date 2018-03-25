//Audio playback helper class (template implementation)
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <cassert>
#include <thread> //TODO: Add <atomic> and make fields used in playback-thread atomic

#include "common.hpp"

//#include "player.hpp"

namespace sndutils
{
  using namespace std;

  template<typename T>
  class WaveFormConverter
  {
    friend class WaveFormGenerator<T>;
    
    public:
      WaveFormConverter(WaveFormGenerator<T> &generator, const size_t number_of_channels)
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
          for (size_t channel = 0; channel < this->number_of_channels; channel++) //Fill all channels with the same samples
          {
            for (size_t sample_byte = 0; sample_byte < sample_size; sample_byte++)
            {
              static_assert(sizeof(T) <= sizeof(unsigned long), "T cannot be larger than unsigned long"); //Since unsigned long is used for shifting below, the type's size has to be smaller than that of an unsigned long
              const auto current_value = static_cast<unsigned long>(this->generator.GetNextSample());
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
      
      WaveFormGenerator<T> &generator;
      const size_t number_of_channels;
      const size_t unit_size;
      const size_t buffer_size;
      unsigned char *buffer;
  };

  AudioPlayer::AudioPlayer(const unsigned int sampling_rate, const size_t number_of_channels) : playing(false), paused(false)
  {
    assert(sampling_rate > 0);
    
    sample_format.bits = 16; //TODO: Make variable
    sample_format.channels = static_cast<int>(number_of_channels);
    sample_format.rate = static_cast<int>(sampling_rate);
    sample_format.byte_format = AO_FMT_LITTLE; //Use little endian
    sample_format.matrix = NULL;
    
    ao_initialize();
    const int driver_id = ao_default_driver_id();
    if (!(playback_device = ao_open_live(driver_id, &sample_format, NULL)))
      throw "Could not open playback device";
  }

  template<typename T>
  void AudioPlayer::Play(WaveFormGenerator<T> &generator)
  {
    static_assert(WaveFormConverter<T>::sample_size == 2, "Only 16-bit samples are currently supported");
    if (playing)
      throw "Already playing. Stop playback first.";
    playing = true;
    
    const size_t this_number_of_channels = this->sample_format.channels;
    const bool &this_playing = this->playing;
    const bool &this_paused = this->paused;
    ao_device * const &this_playback_device = this->playback_device;
    worker = thread([&generator, this_number_of_channels, &this_playing, &this_paused, &this_playback_device]
                    ()
                    {
                      WaveFormConverter<T> converter(generator, this_number_of_channels);
                      while (this_playing)
                      {
                        if (this_paused) //Allow other threads to work while waiting for playback to resume
                          this_thread::yield();
                        else
                        {
                          unsigned char *buffer;
                          size_t buffer_size = converter.GetNextSampleBuffer(buffer);
                          static_assert(sizeof(char) == 1 && sizeof(char) == sizeof(unsigned char), "Both, char and unsigned char, must be one byte in size");
                          if (!ao_play(this_playback_device, (char*)buffer, buffer_size)) //TODO: Double-buffering to avoid pauses
                            throw "Playback error. Don't use this instance again.";
                        }
                      }
                    });
  }

  void AudioPlayer::Stop()
  {
    if (!playing)
      return;
    playing = false;
    worker.join();
  }

  void AudioPlayer::Pause()
  {
    if (!playing)
      throw "Not playing. Start playback first.";
    paused = true;
  }

  void AudioPlayer::Resume()
  {
    if (!playing)
      throw "Not playing. Start playback first.";
    paused = false;
  }

  bool AudioPlayer::IsPlaying() const
  {
    return playing;
  }

  bool AudioPlayer::IsPlayingBack() const
  {
    return playing && !paused;
  }

  AudioPlayer::~AudioPlayer()
  {
    Stop();
    ao_close(playback_device);
    ao_shutdown();
  }
}
