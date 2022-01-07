//Illustration of the signal composition through the 1-D IDCT
// Andreas Unterweger, 2017-2022
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

constexpr double coefficients[] { 0.25, 0.5, 0.0, -0.5 };
constexpr size_t N = comutils::arraysize(coefficients); //Number of samples
static_assert(N >= 2, "The number of samples must be at least 2");

template<size_t... Is>
static constexpr std::array<comutils::CosineWaveGenerator<double>, sizeof...(Is)> CreateGenerators(const std::index_sequence<Is...>&)
{
  static_assert(N == sizeof...(Is), "The sampling rate in Hz must be equal to the number of coefficients");
  return {{ comutils::CosineWaveGenerator<double>(0.5 * Is, //0.5 Hz steps
                                        coefficients[Is], //Amplitude (coefficient for this frequency)
                                        true,
                                        2 * M_PI * 0.5 * (0.5 * Is) / N, //Phase shift of 0.5 samples (2pi*0.5*f/(sampling rate) phase)
                                        N) //Sampling rate is N Hz
                                        ... }}; //Continued in this pattern for every available index/coefficient
}

static std::array<comutils::CosineWaveGenerator<double>, N> CreateAllGenerators()
{
  return CreateGenerators(std::make_index_sequence<N>{}); //Create generators by iterating through all (coefficient) array indices
}

struct DCT_data
{
  std::array<comutils::CosineWaveGenerator<double>, N> generators;
  comutils::WaveFormMixer<double, N> mixer;
  
  const std::string window_name;
  const std::string trackbar_name;
  
  DCT_data(const std::string &window_name, const std::string &trackbar_name)
   : generators{CreateAllGenerators()},
     mixer(std::array<comutils::WaveFormGenerator<double>*, N>{}, 1.0, N), //Add waveforms (mixer with weight 1.0); initialize with empty generators at startup (filled later)
     window_name(window_name), trackbar_name(trackbar_name) { }
};

static void AddCurrentWave(const DCT_data &data, const int visible_coefficients, std::vector<imgutils::PointSet> &point_sets)
{
  if (visible_coefficients != N)
  {
    auto generator = data.generators[visible_coefficients];
    std::vector<double> samples(N);
    generator.GetRepresentativeSamples(samples.size(), samples.data());
    imgutils::PointSet point_set(samples, 1, imgutils::Red, false, true); //No lines, but samples
    point_set.line_width = 3;
    point_sets.push_back(point_set);

    constexpr double plot_frequency = 100.0; //Sampling rate is 100 Hz
    comutils::CosineWaveGenerator<double> function_generator(generator.GetFrequency(), generator.GetAmplitude(), true, generator.GetInitialPhase(), plot_frequency * N); //Sampling rate is N*100 Hz
    std::vector<double> function_samples(plot_frequency * N);
    function_generator.GetRepresentativeSamples(function_samples.size(), function_samples.data());
    point_sets.push_back(imgutils::PointSet(function_samples, 1 / plot_frequency, imgutils::Red, true, false)); //10 ms steps; no samples, but lines
  }
}

static void AddSumWave(DCT_data &data, const int visible_coefficients, std::vector<imgutils::PointSet> &point_sets)
{
  for (size_t i = 0; i < static_cast<size_t>(visible_coefficients); i++)
    data.mixer.SetGenerator(i, &data.generators[i]);
  for (size_t i = static_cast<size_t>(visible_coefficients); i < N; i++) //Disable remaining coefficients
    data.mixer.SetGenerator(i, nullptr);
    
  std::vector<double> added_samples(N);
  data.mixer.GetRepresentativeSamples(added_samples.size(), added_samples.data());
  imgutils::PointSet point_set(added_samples, 1, imgutils::Purple, false, true);
  point_set.line_width = 3;
  point_sets.push_back(point_set); //No lines, but samples
}

static cv::Mat PlotWaves(DCT_data &data, const int visible_coefficients)
{
  std::vector<imgutils::PointSet> point_sets;
  AddSumWave(data, visible_coefficients, point_sets); //Add old sum first so that it is overdrawn if necessary
  AddCurrentWave(data, visible_coefficients, point_sets);
  imgutils::Plot plot(point_sets, false); //No autoscaling
  plot.SetVisibleRange(cv::Point2d(0, -1), cv::Point2d(N, 1)); //TODO: Set proper range to allow for potentially larger intermediate values
  plot.SetAxesLabels("n", "X(n)");
  cv::Mat_<cv::Vec3b> image;
  plot.DrawTo(image);
  return image;
}

static cv::Mat PlotSpectrum(const int visible_coefficients)
{
  size_t displayed_coefficients = std::min(N, static_cast<size_t>(visible_coefficients) + 1); //First few coefficients plus the next one (except for the highest value - here, there is no next coefficient and all are shown again)
  std::vector<imgutils::PointSet> separate_coefficients;
  for (size_t i = 0; i < displayed_coefficients; i++)
  {
    imgutils::PointSet point_set({ cv::Point2d(i, coefficients[i]) }, i == static_cast<size_t>(visible_coefficients) ? imgutils::Red : imgutils::Purple, false, true); //No lines, but samples
    if (i == static_cast<unsigned int>(visible_coefficients))
      point_set.line_width = 3;
    separate_coefficients.push_back(point_set);
  }
    
  imgutils::Plot plot(separate_coefficients, false); //No autoscaling
  plot.SetVisibleRange(cv::Point2d(0, -1), cv::Point2d(N - 1, 1));
  plot.SetAxesLabels("k", "Y(k)");
  cv::Mat_<cv::Vec3b> image;
  plot.DrawTo(image);
  return image;
}

static void ShowControls()
{
  constexpr auto window_name = "Number of DCT components";
  cv::namedWindow(window_name);
  cv::moveWindow(window_name, 0, 0);
  constexpr auto trackbar_name = "Components";
  static DCT_data data(window_name, trackbar_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  cv::createTrackbar(trackbar_name, window_name, nullptr, N, [](const int visible_coefficients, void * const user_data)
                                                                {
                                                                  auto &data = *(static_cast<DCT_data*>(user_data));
                                                                  const cv::Mat wave_image = PlotWaves(data, visible_coefficients);
                                                                  const cv::Mat spectrum_image = PlotSpectrum(visible_coefficients);
                                                                  const cv::Mat combined_image = imgutils::CombineImages({wave_image, spectrum_image}, imgutils::CombinationMode::Horizontal);
                                                                  cv::imshow(data.window_name, combined_image);
                                                                }, static_cast<void*>(&data));
  cv::setTrackbarPos(trackbar_name, window_name, N); //Implies cv::imshow with all visible components
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates a signal's composition by its 1-D DCT components." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowControls();
  cv::waitKey(0);
  return 0;
}
