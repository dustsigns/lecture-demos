//Illustration of the signal composition through the 1-D IDCT
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <limits>
#include <array>
#include <vector>
#include <utility>
#include <limits>
#include <cassert>

#include <opencv2/core.hpp>

#include "common.hpp"
#include "coswave.hpp"
#include "mixer.hpp"
#include "plot.hpp"
#include "colors.hpp"
#include "format.hpp"
#include "combine.hpp"
#include "window.hpp"

template<size_t N>
class DCT_data
{
  static_assert(N >= 2, "The number of coefficients must be at least 2");
  protected:
    imgutils::Window window;
    
    using TrackBarType = imgutils::TrackBar<DCT_data&>;
    TrackBarType components_trackbar;

    const std::array<double, N> coefficients;
    
    using GeneratorType = std::unique_ptr<comutils::CosineWaveGenerator<double>>;
    std::array<GeneratorType, N> generators; //Generators for each coefficient
    
    std::array<std::vector<double>, N + 1> sum_wave_forms; //The wave forms for the sums of the first N - 1 coefficients, including the "empty" sum for zero coefficients
    double min_amplitude = std::numeric_limits<double>::max();
    double max_amplitude = std::numeric_limits<double>::lowest();
  
    void AddCurrentWave(const int visible_coefficients, std::vector<imgutils::PointSet> &point_sets)
    {
      if (visible_coefficients != N)
      {
        const auto &generator = *generators[visible_coefficients];
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

    void AddSumWave(const int visible_coefficients, std::vector<imgutils::PointSet> &point_sets)
    {
      const auto &added_samples = sum_wave_forms[visible_coefficients];
      imgutils::PointSet point_set(added_samples, 1, imgutils::Purple, false, true);
      point_set.line_width = 3;
      point_sets.push_back(point_set); //No lines, but samples
    }

    cv::Mat PlotWaves(const int visible_coefficients)
    {
      std::vector<imgutils::PointSet> point_sets;
      AddSumWave(visible_coefficients, point_sets); //Add old sum first so that it is overdrawn if necessary
      AddCurrentWave(visible_coefficients, point_sets);
      imgutils::Plot plot(point_sets, false); //No autoscaling
      plot.SetVisibleRange(cv::Point2d(0, min_amplitude), cv::Point2d(N, max_amplitude));
      plot.SetAxesLabels("n", "X(n)");
      cv::Mat_<cv::Vec3b> image;
      plot.DrawTo(image);
      return image;
    }

    cv::Mat PlotSpectrum(const int visible_coefficients)
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
      plot.SetVisibleRange(cv::Point2d(0, min_amplitude), cv::Point2d(N - 1, max_amplitude));
      plot.SetAxesLabels("k", "Y(k)");
      cv::Mat_<cv::Vec3b> image;
      plot.DrawTo(image);
      return image;
    }
  
    static void UpdateImage(DCT_data &data)
    {
      const auto visible_coefficients = data.components_trackbar.GetValue();
      const cv::Mat wave_image = data.PlotWaves(visible_coefficients);
      const cv::Mat spectrum_image = data.PlotSpectrum(visible_coefficients);
      const cv::Mat combined_image = imgutils::CombineImages({wave_image, spectrum_image}, imgutils::CombinationMode::Horizontal);
      data.window.UpdateContent(combined_image);
    }
    
    void InitializeWaveForms()
    {
      comutils::WaveFormMixer<double, N> mixer(std::array<comutils::WaveFormGenerator<double>*, N>{}, 1.0, N); //For adding waveforms (mixer with weight 1.0); initialize with empty generators at first (filled later)
      for (size_t i = 0; i < sum_wave_forms.size(); i++)
      {
        if (i != 0) //The very first sum wave is "empty"
        {
          const auto j = i - 1;
          generators[j] = std::make_unique<comutils::CosineWaveGenerator<double>>(0.5 * j, //0.5 Hz steps
                                                                                  coefficients[j], //Amplitude (coefficient for this frequency)
                                                                                  true,
                                                                                  2 * M_PI * 0.5 * (0.5 * j) / N, //Phase shift of 0.5 samples (2pi*0.5*f/(sampling rate) phase)
                                                                                  N); //Sampling rate is N Hz
          mixer.SetGenerator(j, generators[j].get());
        }
        sum_wave_forms[i].resize(N);
        mixer.GetRepresentativeSamples(N, sum_wave_forms[i].data());
        const auto [min_sample_ptr, max_sample_ptr] = std::minmax_element(sum_wave_forms[i].begin(), sum_wave_forms[i].end());
        min_amplitude = std::min(min_amplitude, *min_sample_ptr);
        max_amplitude = std::max(max_amplitude, *max_sample_ptr);
      }
      const auto [min_coeff_ptr, max_coeff_ptr] = std::minmax_element(std::begin(coefficients), std::end(coefficients));
      min_amplitude = std::min(min_amplitude, *min_coeff_ptr);
      max_amplitude = std::max(max_amplitude, *max_coeff_ptr);
    }
  
    static constexpr auto window_name = "Number of DCT components";
    static constexpr auto components_trackbar_name = "Components";
  public:
    DCT_data(const std::array<double, N> &coefficients)
     : window(window_name),
       components_trackbar(components_trackbar_name, window, N, 0, N, UpdateImage, *this), //Show all components by default
       coefficients(coefficients)
    {
      InitializeWaveForms();
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage()
{
  constexpr std::array coefficients { 0.25, 0.5, 0.0, -0.5 };
  DCT_data data(coefficients);
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates a signal's composition by its 1-D DCT components." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowImage();
  return 0;
}
