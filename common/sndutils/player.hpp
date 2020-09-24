//Audio playback helper class (header)
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <atomic>
#include <thread>

#include <ao/ao.h>

#include "wavegen.hpp"

namespace sndutils
{
  using namespace std;
  
  using namespace comutils;
  
  //Plays back raw audio streams on the default playback device
  class AudioPlayer
  {  
    public:
      //Constructs a new instance of AudioPlayer with the given playback device parameters. Make sure that the used device is configured accordingly.
      AudioPlayer(const unsigned int sampling_rate = 48000, const size_t number_of_channels = 2);
      ~AudioPlayer();
      
      //Plays back the wave form produced by the specified generator asynchronously until Stop() is called
      template<typename T> void Play(WaveFormGenerator<T> &generator);
      //Halts previously started playback
      void Stop();
      //Pauses previusly started playback
      void Pause();
      //Resumes previously started playback
      void Resume();
      //Returns whether playback has been started (even when currently paused)
      bool IsPlaying() const;
      //Returns whether there is playback at the moment
      bool IsPlayingBack() const;
    
    private:
      ao_sample_format sample_format;
      ao_device *playback_device;
      atomic<bool> playing;
      atomic<bool> paused;
      thread worker;
  };
}

#include "player.impl.hpp"
