//Illustration of the signal composition through the 1-D IDCT
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <limits>
#include <array>
#include <vector>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"
#include "coswave.hpp"
#include "mixer.hpp"
#include "plot.hpp"
#include "colors.hpp"
#include "format.hpp"
#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace comutils;
using namespace sndutils; //TODO: Remove dependency by moving generators?
using namespace imgutils;

constexpr double coefficients[] { 0.25, 0.5, 0.0, -0.5 };
constexpr size_t N = arraysize(coefficients); //Number of samples
static_assert(N >= 2, "The number of samples must be at least 2");

template<size_t... Is>
static constexpr array<CosineWaveGenerator<double>, sizeof...(Is)> CreateGenerators(const index_sequence<Is...>&)
{
  static_assert(N == sizeof...(Is), "The sampling rate in Hz must be equal to the number of coefficients");
  return {{ CosineWaveGenerator<double>(0.5 * Is, //0.5 Hz steps
                                        coefficients[Is], //Amplitude (coefficient for this frequency)
                                        true,
                                        2 * M_PI * 0.5 * (0.5 * Is) / N, //Phase shift of 0.5 samples (2pi*0.5*f/(sampling rate) phase)
                                        N) //Sampling rate is N Hz
                                        ... }}; //Continued in this pattern for every available index/coefficient
}

static array<CosineWaveGenerator<double>, N> CreateAllGenerators()
{
  return CreateGenerators(make_index_sequence<N>{}); //Create generators by iterating through all (coefficient) array indices
}

typedef struct DCT_data
{
  int visible_coefficients;
  array<CosineWaveGenerator<double>, N> generators;
  WaveFormMixer<double, N> mixer;
  
  const string window_name;
  const string trackbar_name;
  
  DCT_data(const string &window_name, const string &trackbar_name)
   : visible_coefficients(0),
     generators{CreateAllGenerators()},
     mixer(array<WaveFormGenerator<double>*, N>{}, 1.0, N), //Add waveforms (mixer with weight 1.0); initialize with empty generators at startup (filled later)
     window_name(window_name), trackbar_name(trackbar_name) { }
} DCT_data;

static void AddCurrentWave(const DCT_data &data, vector<PointSet> &point_sets)
{
  if (data.visible_coefficients != N)
  {
    auto generator = data.generators[data.visible_coefficients];
    vector<double> samples(N);
    generator.GetRepresentativeSamples(samples.size(), samples.data());
    PointSet point_set(samples, 1, Red, false, true); //No lines, but samples
    point_set.line_width = 3;
    point_sets.push_back(point_set);

    constexpr double plot_frequency = 100.0; //Sampling rate is 100 Hz
    CosineWaveGenerator<double> function_generator(generator.GetFrequency(), generator.GetAmplitude(), true, generator.GetInitialPhase(), plot_frequency * N); //Sampling rate is N*100 Hz
    vector<double> function_samples(plot_frequency * N);
    function_generator.GetRepresentativeSamples(function_samples.size(), function_samples.data());
    point_sets.push_back(PointSet(function_samples, 1 / plot_frequency, Red, true, false)); //10 ms steps; no samples, but lines
  }
}

static void AddSumWave(DCT_data &data, vector<PointSet> &point_sets)
{
  for (size_t i = 0; i < (size_t)data.visible_coefficients; i++)
    data.mixer.SetGenerator(i, &data.generators[i]);
  for (size_t i = (size_t)data.visible_coefficients; i < N; i++) //Disable remaining coefficients
    data.mixer.SetGenerator(i, NULL);
    
  vector<double> added_samples(N);
  data.mixer.GetRepresentativeSamples(added_samples.size(), added_samples.data());
  PointSet point_set(added_samples, 1, Purple, false, true);
  point_set.line_width = 3;
  point_sets.push_back(point_set); //No lines, but samples
}

static Mat PlotWaves(DCT_data &data)
{
  vector<PointSet> point_sets;
  AddSumWave(data, point_sets); //Add old sum first so that it is overdrawn if necessary
  AddCurrentWave(data, point_sets);
  Plot plot(point_sets, false); //No autoscaling
  plot.SetVisibleRange(Point2d(0, -1), Point2d(N, 1)); //TODO: Set proper range to allow for potentially larger intermediate values
  plot.SetAxesLabels("n", "X(n)");
  Mat_<Vec3b> image;
  plot.DrawTo(image);
  return image;
}

static Mat PlotSpectrum(const DCT_data &data)
{
  size_t displayed_coefficients = min(N, (size_t)data.visible_coefficients + 1); //First few coefficients plus the next one (except for the highest value - here, there is no next coefficient and all are shown again)
  vector<PointSet> separate_coefficients;
  for (size_t i = 0; i < displayed_coefficients; i++)
  {
    PointSet point_set({ Point2d(i, coefficients[i]) }, i == (size_t)data.visible_coefficients ? Red : Purple, false, true); //No lines, but samples
    if (i == static_cast<unsigned int>(data.visible_coefficients))
      point_set.line_width = 3;
    separate_coefficients.push_back(point_set);
  }
    
  Plot plot(separate_coefficients, false); //No autoscaling
  plot.SetVisibleRange(Point2d(0, -1), Point2d(N - 1, 1));
  plot.SetAxesLabels("k", "Y(k)");
  Mat_<Vec3b> image;
  plot.DrawTo(image);
  return image;
}

static void ShowControls()
{
  constexpr auto window_name = "Number of DCT components";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  constexpr auto trackbar_name = "Components";
  static DCT_data data(window_name, trackbar_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  createTrackbar(trackbar_name, window_name, &data.visible_coefficients, N, [](const int, void * const user_data)
                                                                              {
                                                                                auto &data = *((DCT_data * const)user_data);
                                                                                const Mat wave_image = PlotWaves(data);
                                                                                const Mat spectrum_image = PlotSpectrum(data);
                                                                                const Mat combined_image = CombineImages({wave_image, spectrum_image}, Horizontal);
                                                                                imshow(data.window_name, combined_image);
                                                                              }, (void*)&data);
  setTrackbarPos(trackbar_name, window_name, N); //Implies imshow with all visible components
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates a signal's composition by its 1-D DCT components." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  ShowControls();
  waitKey(0);
  return 0;
}
