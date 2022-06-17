//Illustration of frequency masking
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <limits>
#include <array>
#include <algorithm>
#include <set>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"
#include "sinewave.hpp"
#include "mixer.hpp"
#include "player.hpp"
#include "math.hpp"
#include "plot.hpp"
#include "colors.hpp"
#include "combine.hpp"
#include "window.hpp"

template<size_t N>
class audio_data
{
  static_assert(N >= 2, "The number of frequencies must be at least 2");
  protected:
    const std::array<unsigned int, N> frequencies;
    const double max_frequency;
    const std::array<unsigned int, N> default_levels;
  
    using audio_type = short; //May be signed char (for 8 bits), short (16) or int (32)  
    using GeneratorType = comutils::SineWaveGenerator<audio_type>;
    std::array<std::unique_ptr<GeneratorType>, N> generators; //Generators for each frequency
    using MixerType = comutils::WaveFormMixer<audio_type, N>;
    std::unique_ptr<MixerType> mixer;
    sndutils::AudioPlayer<audio_type> player;
  
    imgutils::Window window;
    
    using TrackBarType = imgutils::TrackBar<audio_data&>;
    std::array<std::unique_ptr<TrackBarType>, N> level_trackbars;
    using CheckBoxType = imgutils::CheckBox<audio_data&>;
    CheckBoxType mute_checkbox;
     
    std::array<int, N> ResetMixer()
    {
      if (player.IsPlaying())
        player.Stop();
      std::array<int, N> levels_percent;
      std::transform(std::begin(level_trackbars), std::end(level_trackbars), levels_percent.begin(),
                     [](const std::unique_ptr<TrackBarType> &trackbar)
                       {
                         const auto level = trackbar->GetValue();
                         return level;
                       });
      for (size_t i = 0; i < N; i++)
      {
        const auto amplitude = comutils::GetValueFromLevel(-levels_percent[i], 1); //Interpret trackbar position as (negative) level in dB (due to attenuation). The reference value is 1 since the amplitude is specified relatively, i.e., between 0 and 1.
        generators[i]->SetAmplitude(amplitude);
      }
      player.Play(*mixer);
      return levels_percent;
    }
    
    cv::Mat PlotWaves()
    {
      constexpr size_t sampling_frequency = 48000;
      const size_t displayed_samples = 5 * (sampling_frequency / max_frequency) + 1; //About five periods of the higher-frequency wave form plus one sample
      std::array<std::vector<audio_type>, N + 1> samples;
      for (auto &sample_vector : samples)
        sample_vector.resize(displayed_samples);
      for (size_t i = 0; i < N; i++)
        generators[i]->GetRepresentativeSamples(samples[i].size(), samples[i].data());
      mixer->GetRepresentativeSamples(samples[N].size(), samples[N].data());
      
      std::vector<imgutils::PointSet> pointsets;
      for (size_t i = 0; i < N + 1; i++)
      {
        const auto color = i == 0 ? imgutils::Red : (i == N ? imgutils::Purple : imgutils::Blue);
        pointsets.emplace_back(samples[i], 1, color);
      }
      imgutils::Plot plot(pointsets);
      plot.SetAxesLabels("t [ms]", "I(t)");
      imgutils::Tick::GenerateTicks(plot.x_axis_ticks, 0, displayed_samples, 0.001 * sampling_frequency, 1, 0, 1000.0 / sampling_frequency); //Mark and label every ms with no decimal places and a relative scale (1000 for ms)
      plot.x_axis_ticks.pop_back(); //Remove last tick and label so that the axis label is not overwritten
      imgutils::Tick::GenerateTicks(plot.y_axis_ticks, std::numeric_limits<audio_type>::lowest() + 1, std::numeric_limits<audio_type>::max(), std::numeric_limits<audio_type>::max() / 2.0, 1, 1, 1.0 / std::numeric_limits<audio_type>::max()); //Mark and label every 0.5 units (0-1) with 1 decimal place and a relative scale (where the maximum is 1)
      cv::Mat_<cv::Vec3b> image;
      plot.DrawTo(image);
      return image;
    }
     
    cv::Mat PlotSpectrum(const std::array<int, N> &levels_percent)
    {
      const auto max_displayed_frequency = 1.5 * max_frequency;
      std::vector<imgutils::PointSet> pointsets;
      for (size_t i = 0; i < N; i++)
      {
        const auto color = i == 0 ? imgutils::Red : imgutils::Blue;
        imgutils::PointSet pointset({cv::Point2d(frequencies[i], -levels_percent[i])}, color, false, true); //No lines, but samples
        pointsets.push_back(std::move(pointset));
      }
      imgutils::Plot plot(pointsets);
      plot.SetAxesLabels("f [Hz]", "A(f) [dB]");
      imgutils::Tick::GenerateTicks(plot.x_axis_ticks, 0, max_displayed_frequency, 100, 2); //Mark every 100 Hz, label every 200 Hz (0 - max. frequency)
      imgutils::Tick::GenerateTicks(plot.y_axis_ticks, 0, -static_cast<int>(max_level), -10, 2); //Mark every 10 dB, label every 20 dB (0 - -100)
      cv::Mat_<cv::Vec3b> image;
      plot.DrawTo(image);
      return image;
    }
     
    static void UpdateImage(audio_data &data)
    {
      const auto levels_percent = data.ResetMixer();
      const cv::Mat wave_image = data.PlotWaves();
      const cv::Mat spectrum_image = data.PlotSpectrum(levels_percent);
      const cv::Mat combined_image = imgutils::CombineImages({wave_image, spectrum_image}, imgutils::CombinationMode::Horizontal);
      data.window.UpdateContent(combined_image);
    }

    static void Mute(audio_data &data)
    {
      data.player.Pause();
    }
    
    static void Unmute(audio_data &data)
    {
      data.player.Resume();
    }

    static constexpr auto trackbar_unit_name = " Hz level [-dB]";
    
    void InitializeGenerators()
    {
      std::transform(std::begin(frequencies), std::end(frequencies), generators.begin(),
                     [](const double frequency)
                       {
                         return std::make_unique<GeneratorType>(frequency);
                       });
      std::array<comutils::WaveFormGenerator<audio_type>*, N> generator_pointers;
      for (size_t i = 0; i < N; i++)
        generator_pointers[i] = generators[i].get();
      mixer = std::make_unique<MixerType>(generator_pointers);
    }
    
    void AddControls()
    {
      for (size_t i = 0; i < N; i++)
      {
        const auto trackbar_name = std::to_string(frequencies[i]) + trackbar_unit_name;
        level_trackbars[i] = std::make_unique<TrackBarType>(trackbar_name, window, max_level, 0, default_levels[i], UpdateImage, *this);
      }
    }
    
    static constexpr auto window_name = "Attenuation";
    static constexpr auto mute_checkbox_name = "Mute";
  public:
    static constexpr unsigned int max_level = 100;
    
    audio_data(const std::array<unsigned int, N> &frequencies, const std::array<unsigned int, N> &default_levels)
     : frequencies(frequencies),
       max_frequency(*std::max_element(std::begin(frequencies), std::end(frequencies))),
       default_levels(default_levels),
       window(window_name),
       mute_checkbox(mute_checkbox_name, window, false, Mute, Unmute, *this) //Unmuted by default
    {
      assert(std::set(frequencies.begin(), frequencies.end()).size() == frequencies.size()); //Check if any frequency occurs twice
      const auto max_default_level = *std::max_element(std::begin(default_levels), std::end(default_levels));
      assert(max_default_level <= max_level);
      InitializeGenerators();
      AddControls();
      ResetMixer();
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage()
{
  constexpr std::array frequencies { 400U, 440U };
  constexpr std::array default_levels { 0U, 20U };
  audio_data data(frequencies, default_levels);
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates frequency masking at different intensities." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowImage();
  return 0;
}
