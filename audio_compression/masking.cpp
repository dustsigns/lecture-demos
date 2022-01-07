//Illustration of frequency masking
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <limits>
#include <array>

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

static constexpr unsigned int frequencies[] {400, 440};
static_assert(comutils::arraysize(frequencies) == 2, "This application can only illustrate masking for two frequencies in total");
static_assert(frequencies[1] > frequencies[0], "The second frequency has to be larger than the first");

using audio_type = short; //May be signed char (for 8 bits), short (16) or int (32)

struct audio_data
{
  comutils::SineWaveGenerator<audio_type> generators[2];
  comutils::WaveFormMixer<audio_type, 2> mixer;
  sndutils::AudioPlayer<audio_type> player;
  
  const std::string window_name;
  
  audio_data(const std::string &window_name)
   : generators{comutils::SineWaveGenerator<audio_type>(frequencies[0]), comutils::SineWaveGenerator<audio_type>(frequencies[1])},
     mixer({&generators[0], &generators[1]}),
     window_name(window_name) { }
};

static cv::Mat PlotWaves(const audio_data &data)
{
  constexpr size_t sampling_frequency = 48000;
  constexpr size_t displayed_samples = 5 * (sampling_frequency / frequencies[0]) + 1; //About five periods of the higher-frequency wave form plus one sample
  std::array<std::vector<audio_type>, 3> samples { std::vector<audio_type>(displayed_samples), std::vector<audio_type>(displayed_samples), std::vector<audio_type>(displayed_samples) };
  for (const auto &i : {0, 1})
    data.generators[i].GetRepresentativeSamples(samples[i].size(), samples[i].data());
  data.mixer.GetRepresentativeSamples(samples[2].size(), samples[2].data());
  
  imgutils::Plot plot({imgutils::PointSet(samples[0], 1, imgutils::Red),
                       imgutils::PointSet(samples[1], 1, imgutils::Blue),
                       imgutils::PointSet(samples[2], 1, imgutils::Purple)});
  plot.SetAxesLabels("t [ms]", "I(t)");
  imgutils::Tick::GenerateTicks(plot.x_axis_ticks, 0, displayed_samples, 0.001 * sampling_frequency, 1, 0, 1000.0 / sampling_frequency); //Mark and label every ms with no decimal places and a relative scale (1000 for ms)
  plot.x_axis_ticks.pop_back(); //Remove last tick and label so that the axis label is not overwritten
  imgutils::Tick::GenerateTicks(plot.y_axis_ticks, std::numeric_limits<audio_type>::lowest() + 1, std::numeric_limits<audio_type>::max(), std::numeric_limits<audio_type>::max() / 2.0, 1, 1, 1.0 / std::numeric_limits<audio_type>::max()); //Mark and label every 0.5 units (0-1) with 1 decimal place and a relative scale (where the maximum is 1)
  cv::Mat_<cv::Vec3b> image;
  plot.DrawTo(image);
  return image;
}

static cv::Mat PlotSpectrum(int (&levels_percent)[2])
{
  constexpr auto max_frequency = frequencies[1] * 1.5;
  imgutils::Plot plot({imgutils::PointSet({cv::Point2d(frequencies[0], -levels_percent[0])}, imgutils::Red, false, true), //No lines, but samples
                       imgutils::PointSet({cv::Point2d(frequencies[1], -levels_percent[1])}, imgutils::Blue, false, true)});
  plot.SetAxesLabels("f [Hz]", "A(f) [dB]");
  imgutils::Tick::GenerateTicks(plot.x_axis_ticks, 0, max_frequency, 100, 2); //Mark every 100 Hz, label every 200 Hz (0 - max. frequency)
  imgutils::Tick::GenerateTicks(plot.y_axis_ticks, 0, -100, -10, 2); //Mark every 10 dB, label every 20 dB (0 - -100)
  cv::Mat_<cv::Vec3b> image;
  plot.DrawTo(image);
  return image;
}

static std::string GetTrackbarName(const int i)
{
  constexpr auto unit_name = " Hz level [-dB]";
  const auto trackbar_name = std::to_string(frequencies[i]) + unit_name;
  return trackbar_name;
}

static void TrackbarEvent(const int, void* user_data)
{
  auto &data = *(static_cast<audio_data*>(user_data));
  if (data.player.IsPlaying())
    data.player.Stop();
  int levels_percent[2];
  for (const auto &i : {0, 1})
  {
    levels_percent[i] = cv::getTrackbarPos(GetTrackbarName(i), data.window_name);
    const auto amplitude = comutils::GetValueFromLevel(-levels_percent[i], 1); //Interpret trackbar position as (negative) level in dB (due to attenuation). The reference value is 1 since the amplitude is specified relatively, i.e., between 0 and 1.
    data.generators[i].SetAmplitude(amplitude);
  }
  data.player.Play(data.mixer);
  const cv::Mat wave_image = PlotWaves(data);
  const cv::Mat spectrum_image = PlotSpectrum(levels_percent);
  const cv::Mat combined_image = imgutils::CombineImages({wave_image, spectrum_image}, imgutils::CombinationMode::Horizontal);
  cv::imshow(data.window_name, combined_image);
}

static void ShowControls()
{
  constexpr auto window_name = "Attenuation";
  cv::namedWindow(window_name);
  cv::moveWindow(window_name, 0, 0);
  static audio_data data(window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  for (const auto &i : {0, 1})
  {
    const auto trackbar_name = GetTrackbarName(i);
    cv::createTrackbar(trackbar_name, data.window_name, nullptr, 100, TrackbarEvent, static_cast<void*>(&data));
  }
  cv::createButton("Mute", [](const int, void * const user_data)
                             {
                               auto &data = *(static_cast<audio_data*>(user_data));
                               if (data.player.IsPlayingBack())
                                 data.player.Pause();
                               else
                                 data.player.Resume();
                             }, static_cast<void*>(&data), cv::QT_CHECKBOX);
  cv::setTrackbarPos(GetTrackbarName(0), window_name, 0);
  cv::setTrackbarPos(GetTrackbarName(1), window_name, 20); //Implies cv::imshow with second level at -20 dB
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates frequency masking at different intensities." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowControls();
  cv::waitKey(0);
  return 0;
}
