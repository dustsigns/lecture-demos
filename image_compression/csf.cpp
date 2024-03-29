//Illustration of the contrast sensitivity function
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <cmath>

#include <opencv2/core.hpp>

#include "window.hpp"

static double ExponentialProgression(const double minimum, const double maximum, const unsigned int steps, const unsigned int step)
{
  assert(minimum < maximum);
  assert(steps > 0);
  assert(step <= steps);
  return minimum * pow(maximum / minimum, static_cast<double>(step) / steps);
}

static cv::Mat GenerateCSFImage()
{
  constexpr int width = 800;
  constexpr int height = 600;
  constexpr double max_brightness = 255;
  static_assert(max_brightness >= 0 && max_brightness <= 255, "Maximum brightness must fit into 8 bits (unsigned char)");
  cv::Mat_<unsigned char> image(height, width, static_cast<unsigned char>(max_brightness));
  image.forEach([](unsigned char &value, const int position[])
                  {
                    constexpr double min_frequency = 1;
                    constexpr double max_frequency = width / 10; //TODO: width / 2 - 1 would be the Shannon limit; is there a sanity check for aliasing when frequency increases within one period? If width is too small, even the current value of 100 is too large
                    constexpr double min_amplitude = 0.5; //Resolution limit (after rounding)
                    constexpr double max_amplitude = static_cast<double>(max_brightness) / 2;
                    static_assert(min_amplitude < max_amplitude, "Minimum amplitude must be smaller than maximum amplitude");
                    constexpr double offset = max_brightness - max_amplitude;
                    const int y = position[0];
                    const int x = position[1];
                    const double amplitude = ExponentialProgression(min_amplitude, max_amplitude, height, y + 1);
                    const double phase = static_cast<double>(x) / width;
                    const double frequency = ExponentialProgression(min_frequency, max_frequency, width, x + 1);
                    value = offset + amplitude * sin(2 * M_PI * phase * frequency);
                  });
  return image;
}

static void ShowContrastSensitivityFunction()
{
  constexpr auto window_name = "Contrast sensitivity function";
  const cv::Mat csf_image = GenerateCSFImage();
  imgutils::Window window(window_name, csf_image);
  window.ShowInteractive();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    std::cout << "Illustrates the contrast sensitivity function." << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    return 1;
  }
  ShowContrastSensitivityFunction();
  return 0;
}
