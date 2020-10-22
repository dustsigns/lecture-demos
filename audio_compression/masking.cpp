//Illustration of frequency masking
// Andreas Unterweger, 2017-2020
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

using namespace std;

using namespace cv;

using namespace comutils;
using namespace sndutils;
using namespace imgutils;

static constexpr unsigned int frequencies[] {400, 440};
static_assert(arraysize(frequencies) == 2, "This application can only illustrate masking for two frequencies in total");
static_assert(frequencies[1] > frequencies[0], "The second frequency has to be larger than the first");

using audio_type = short; //May be signed char (for 8 bits), short (16) or int (32)

struct audio_data
{
  int levels_percent[2];
  SineWaveGenerator<audio_type> generators[2];
  WaveFormMixer<audio_type, 2> mixer;
  AudioPlayer<audio_type> player;
  
  const string window_name;
  
  audio_data(const string &window_name)
   : levels_percent { 0, 0 },
     generators{SineWaveGenerator<audio_type>(frequencies[0]), SineWaveGenerator<audio_type>(frequencies[1])},
     mixer({&generators[0], &generators[1]}),
     window_name(window_name) { }
};

static Mat PlotWaves(const audio_data &data)
{
  constexpr size_t sampling_frequency = 48000;
  constexpr size_t displayed_samples = 5 * (sampling_frequency / frequencies[0]) + 1; //About five periods of the higher-frequency wave form plus one sample
  array<vector<audio_type>, 3> samples { vector<audio_type>(displayed_samples), vector<audio_type>(displayed_samples), vector<audio_type>(displayed_samples) };
  for (const auto &i : {0, 1})
    data.generators[i].GetRepresentativeSamples(samples[i].size(), samples[i].data());
  data.mixer.GetRepresentativeSamples(samples[2].size(), samples[2].data());
  
  Plot plot({PointSet(samples[0], 1, Red),
             PointSet(samples[1], 1, Blue),
             PointSet(samples[2], 1, Purple)});
  plot.SetAxesLabels("t [ms]", "I(t)");
  Tick::GenerateTicks(plot.x_axis_ticks, 0, displayed_samples, 0.001 * sampling_frequency, 1, 0, 1000.0 / sampling_frequency); //Mark and label every ms with no decimal places and a relative scale (1000 for ms)
  plot.x_axis_ticks.pop_back(); //Remove last tick and label so that the axis label is not overwritten
  Tick::GenerateTicks(plot.y_axis_ticks, numeric_limits<audio_type>::min() + 1, numeric_limits<audio_type>::max(), numeric_limits<audio_type>::max() / 2.0, 1, 1, 1.0 / numeric_limits<audio_type>::max()); //Mark and label every 0.5 units (0-1) with 1 decimal place and a relative scale (where the maximum is 1)
  Mat_<Vec3b> image;
  plot.DrawTo(image);
  return image;
}

static Mat PlotSpectrum(const audio_data &data)
{
  constexpr auto max_frequency = frequencies[1] * 1.5;
  Plot plot({PointSet({Point2d(frequencies[0], -data.levels_percent[0])}, Red, false, true), //No lines, but samples
             PointSet({Point2d(frequencies[1], -data.levels_percent[1])}, Blue, false, true)});
  plot.SetAxesLabels("f [Hz]", "A(f) [dB]");
  Tick::GenerateTicks(plot.x_axis_ticks, 0, max_frequency, 100, 2); //Mark every 100 Hz, label every 200 Hz (0 - max. frequency)
  Tick::GenerateTicks(plot.y_axis_ticks, 0, -100, -10, 2); //Mark every 10 dB, label every 20 dB (0 - -100)
  Mat_<Vec3b> image;
  plot.DrawTo(image);
  return image;
}

template<size_t trackbar_id>
static void TrackbarEvent(const int, void* user_data)
{
  static_assert(trackbar_id == 0 || trackbar_id == 1, "Trackbar ID must be 0 or 1");
  auto &data = *(static_cast<audio_data*>(user_data));
  if (data.player.IsPlaying())
    data.player.Stop();
  for (const auto &i : {0, 1})
  {
    const auto amplitude = GetValueFromLevel(-data.levels_percent[i], 1); //Interpret trackbar position as (negative) level in dB (due to attenuation). The reference value is 1 since the amplitude is specified relatively, i.e., between 0 and 1.
    data.generators[i].SetAmplitude(amplitude);
  }
  data.player.Play(data.mixer);
  const Mat wave_image = PlotWaves(data);
  const Mat spectrum_image = PlotSpectrum(data);
  const Mat combined_image = CombineImages({wave_image, spectrum_image}, Horizontal);
  imshow(data.window_name, combined_image);
}

template<size_t i>
static string AddTrackbar(audio_data &data)
{
  constexpr auto unit_name = " Hz level [-dB]";
  const auto trackbar_name = to_string(frequencies[i]) + unit_name;
  createTrackbar(trackbar_name, data.window_name, &data.levels_percent[i], 100, TrackbarEvent<i>, static_cast<void*>(&data));
  return trackbar_name;
}

static void ShowControls()
{
  constexpr auto window_name = "Attenuation";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  static audio_data data(window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  AddTrackbar<0>(data);
  const auto last_trackbar_name = AddTrackbar<1>(data);
  createButton("Mute", [](const int, void * const user_data)
                       {
                         auto &data = *(static_cast<audio_data*>(user_data));
                         if (data.player.IsPlayingBack())
                           data.player.Pause();
                         else
                           data.player.Resume();
                       }, static_cast<void*>(&data), QT_CHECKBOX);
  setTrackbarPos(last_trackbar_name, window_name, 20); //Implies imshow with second level at -20 dB
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates frequency masking at different intensities." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  ShowControls();
  waitKey(0);
  return 0;
}
