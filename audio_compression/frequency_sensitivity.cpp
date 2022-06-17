//Illustration of frequency-dependent intensity sensitivity
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>

#include "sinewave.hpp"
#include "player.hpp"
#include "math.hpp"
#include "plot.hpp"
#include "colors.hpp"
#include "format.hpp"
#include "window.hpp"

class audio_data
{
  protected:
    using audio_type = short; //May be signed char (for 8 bits), short (16) or int (32)  
    comutils::SineWaveGenerator<audio_type> generator;
    sndutils::AudioPlayer<audio_type> player;
  
    imgutils::Window window;
    
    using TrackBarType = imgutils::TrackBar<audio_data&>;
    TrackBarType frequency_trackbar;
    TrackBarType level_trackbar;
    using CheckBoxType = imgutils::CheckBox<audio_data&>;
    CheckBoxType mute_checkbox;
    
    void ResetGenerator()
    {
      if (player.IsPlaying())
        player.Stop();
      const auto frequency = frequency_trackbar.GetValue();
      const auto level_percent = level_trackbar.GetValue();
      const auto level = comutils::GetValueFromLevel(-level_percent, 1); //Interpret trackbar position as (negative) level in dB (due to attenuation). The reference value is 1 since the amplitude is specified relatively, i.e., between 0 and 1.
      generator.SetFrequency(frequency);
      generator.SetAmplitude(level);
      player.Play(generator);
    }
    
    cv::Mat PlotWaves()
    {
      constexpr size_t sampling_frequency = 48000;
      constexpr size_t displayed_samples = sampling_frequency / 10; //100 ms
      std::vector<audio_type> samples(displayed_samples);
      generator.GetRepresentativeSamples(samples.size(), samples.data());
      imgutils::Plot plot({imgutils::PointSet(samples, 1, imgutils::Blue)});
      plot.SetAxesLabels("t [ms]", "I(t)");
      imgutils::Tick::GenerateTicks(plot.x_axis_ticks, 0, displayed_samples, 0.01 * sampling_frequency, 1, 0, 1000.0 / sampling_frequency); //Mark and label every 10 ms with no decimal places and a relative scale (1000 for ms)
      plot.x_axis_ticks.pop_back(); //Remove last tick and label so that the axis label is not overwritten
      imgutils::Tick::GenerateTicks(plot.y_axis_ticks, std::numeric_limits<audio_type>::lowest() + 1, std::numeric_limits<audio_type>::max(), std::numeric_limits<audio_type>::max() / 2.0, 1, 1, 1.0 / std::numeric_limits<audio_type>::max()); //Mark and label every 0.5 units (0-1) with 1 decimal place and a relative scale (where the maximum is 1)
      cv::Mat_<cv::Vec3b> image;
      plot.DrawTo(image);
      return image;
    }
    
    static void UpdateImage(audio_data &data)
    {
      data.ResetGenerator();
      const cv::Mat image = data.PlotWaves();
      data.window.UpdateContent(image);
    }
    
    static void Mute(audio_data &data)
    {
      data.player.Pause();
    }
    
    static void Unmute(audio_data &data)
    {
      data.player.Resume();
    }
    
    static constexpr auto window_name = "Attenuation";
    static constexpr auto frequency_trackbar_name = "Frequency [Hz]";
    static constexpr auto level_trackbar_name = "Level [-dB]";
    static constexpr auto mute_checkbox_name = "Mute";
  public:
    static constexpr unsigned int default_frequency = 440;
    static constexpr unsigned int max_frequency = 8000;
    static_assert(default_frequency <= max_frequency, "The default frequency cannot exceed the maximum frequency.");
    
    static constexpr unsigned int default_level = 20; //-20 dB
    static constexpr unsigned int max_level = 100;
    static_assert(default_level <= max_level, "The default level must not exceed the maximum level.");
  
    audio_data()
     : generator(default_frequency),
       window(window_name),
       frequency_trackbar(frequency_trackbar_name, window, max_frequency, 0, default_frequency, UpdateImage, *this),
       level_trackbar(level_trackbar_name, window, max_level, 0, default_level, UpdateImage, *this),
       mute_checkbox(mute_checkbox_name, window, false, Mute, Unmute, *this) //Unmuted by default
    {
      ResetGenerator();
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage()
{
  audio_data data;
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates how intensities are perceived differently at different frequencies." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowImage();
  return 0;
}
