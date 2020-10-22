//Illustration of frequency-dependent intensity sensitivity
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "sinewave.hpp"
#include "player.hpp"
#include "math.hpp"
#include "plot.hpp"
#include "colors.hpp"
#include "format.hpp"

using namespace std;

using namespace cv;

using namespace comutils;
using namespace sndutils;
using namespace imgutils;

static constexpr unsigned int default_frequency = 440;
static constexpr unsigned int max_frequency = 8000;
static_assert(default_frequency <= max_frequency, "The default frequency cannot exceed the maximum frequency.");

using audio_type = short; //May be signed char (for 8 bits), short (16) or int (32)

struct audio_data
{
  int frequency;
  int level_percent;
  SineWaveGenerator<audio_type> generator;
  AudioPlayer<audio_type> player;
  
  const string window_name;
  const string frequency_trackbar_name;
  const string level_trackbar_name;
  
  audio_data(const string &window_name, const string &frequency_trackbar_name, const string &level_trackbar_name) 
   : frequency(default_frequency),
     level_percent(0),
     generator(default_frequency),
     window_name(window_name), frequency_trackbar_name(frequency_trackbar_name), level_trackbar_name(level_trackbar_name) { }
};

static void ResetGenerator(audio_data &data)
{
  if (data.player.IsPlaying())
    data.player.Stop();
  const auto frequency = data.frequency;
  const auto level = GetValueFromLevel(-data.level_percent, 1); //Interpret trackbar position as (negative) level in dB (due to attenuation). The reference value is 1 since the amplitude is specified relatively, i.e., between 0 and 1.
  data.generator.SetFrequency(frequency);
  data.generator.SetAmplitude(level);
  data.player.Play(data.generator);
}

static Mat PlotWaves(const audio_data &data)
{
  constexpr size_t sampling_frequency = 48000;
  constexpr size_t displayed_samples = sampling_frequency / 10; //100 ms
  vector<audio_type> samples(displayed_samples);
  data.generator.GetRepresentativeSamples(samples.size(), samples.data());
  Plot plot({PointSet(samples, 1, Blue)});
  plot.SetAxesLabels("t [ms]", "I(t)");
  Tick::GenerateTicks(plot.x_axis_ticks, 0, displayed_samples, 0.01 * sampling_frequency, 1, 0, 1000.0 / sampling_frequency); //Mark and label every 10 ms with no decimal places and a relative scale (1000 for ms)
  plot.x_axis_ticks.pop_back(); //Remove last tick and label so that the axis label is not overwritten
  Tick::GenerateTicks(plot.y_axis_ticks, numeric_limits<audio_type>::min() + 1, numeric_limits<audio_type>::max(), numeric_limits<audio_type>::max() / 2.0, 1, 1, 1.0 / numeric_limits<audio_type>::max()); //Mark and label every 0.5 units (0-1) with 1 decimal place and a relative scale (where the maximum is 1)
  Mat_<Vec3b> image;
  plot.DrawTo(image);
  return image;
}

static void TrackbarEvent(const int, void * const user_data)
{
  auto &data = *(static_cast<audio_data*>(user_data));
  ResetGenerator(data);
  Mat image = PlotWaves(data);
  imshow(data.window_name, image);
}

static void ShowControls()
{
  constexpr auto window_name = "Attenuation";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  constexpr auto frequency_trackbar_name = "Frequency [Hz]";
  constexpr auto level_trackbar_name = "Level [-dB]";
  static audio_data data(window_name, frequency_trackbar_name, level_trackbar_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  createTrackbar(frequency_trackbar_name, window_name, &data.frequency, max_frequency, TrackbarEvent, static_cast<void*>(&data));
  createTrackbar(level_trackbar_name, window_name, &data.level_percent, 100, TrackbarEvent, static_cast<void*>(&data));
  constexpr auto checkbox_name = "Mute";
  createButton(checkbox_name, [](const int, void * const user_data)
                              {
                                auto &data = *(static_cast<audio_data*>(user_data));
                                if (data.player.IsPlayingBack())
                                  data.player.Pause();
                                else
                                  data.player.Resume();
                              }, static_cast<void*>(&data), QT_CHECKBOX);
  setTrackbarPos(level_trackbar_name, window_name, 20); //Implies imshow with level at -20 dB
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates how intensities are perceived differently at different frequencies." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  ShowControls();
  waitKey(0);
  return 0;
}
