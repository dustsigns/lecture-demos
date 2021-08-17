//Illustration of downsampling and upsampling
// Andreas Unterweger, 2017-2021
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"
#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace comutils;
using namespace imgutils;

struct resampling_data
{
  const Mat image;
  int resampling_algorithm;
  
  const string window_name;
  
  static constexpr auto scaling_trackbar_name = "Scaling [%]";
  
  resampling_data(const Mat &image, const string &window_name)
   : image(image),
     resampling_algorithm(INTER_NEAREST), window_name(window_name) { }
};

constexpr pair<const char*, int> resampling_algorithms[] {make_pair("Nearest neighbor", INTER_NEAREST),
                                                          make_pair("Bilinear", INTER_LINEAR),
                                                          make_pair("Lanczos-4", INTER_LANCZOS4)};

static void ShowResampledImages(const int scaling_factor_percent, void * const user_data)
{
  auto &data = *(static_cast<const resampling_data*>(user_data));
  const Mat &image = data.image;
  const double scaling_factor = sqrt(scaling_factor_percent / 100.0);
  Mat downsampled_image;
  resize(image, downsampled_image, Size(), scaling_factor, scaling_factor, data.resampling_algorithm);
  Mat upsampled_image;
  resize(downsampled_image, upsampled_image, image.size(), 0, 0, data.resampling_algorithm);
  const Mat combined_image = CombineImages({image, upsampled_image, downsampled_image}, Horizontal);
  imshow(data.window_name, combined_image);
}

template<int A>
static void SetResamplingAlgorithm(const int state, void * const user_data)
{
  if (!state) //Ignore radio button events where the button becomes unchecked
    return;
  auto &data = *(static_cast<resampling_data*>(user_data));
  data.resampling_algorithm = A;
  const int scaling_factor_percent = getTrackbarPos(data.scaling_trackbar_name, data.window_name);
  ShowResampledImages(scaling_factor_percent, user_data);
}

template<size_t... Is>
static void CreateButtons(void * const data, const index_sequence<Is...>&)
{
  constexpr auto index_limit = arraysize(resampling_algorithms);
  static_assert(sizeof...(Is) <= index_limit, "Number of array indices is out of bounds");
  ((void)createButton(resampling_algorithms[Is].first, SetResamplingAlgorithm<resampling_algorithms[Is].second>, data, QT_RADIOBOX, Is == 0), ...); //Make first radio button checked
}

static void CreateAllButtons(void * const data)
{
  constexpr auto N = arraysize(resampling_algorithms);
  CreateButtons(data, make_index_sequence<N>{}); //Create N buttons with callbacks for every index
}

static void AddControls(resampling_data &data)
{
  createTrackbar(data.scaling_trackbar_name, data.window_name, nullptr, 100, ShowResampledImages, static_cast<void*>(&data));
  setTrackbarMin(data.scaling_trackbar_name, data.window_name, 1);
  CreateAllButtons(static_cast<void*>(&data));
}

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "Original vs. resampled (incl. intermediate downsampled)";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  static resampling_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  AddControls(data);
  setTrackbarPos(data.scaling_trackbar_name, window_name, 50); //Implies imshow with 50% scaling factor
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the effect of resampling." << endl;
    cout << "Usage: " << argv[0] << " <input image>" << endl;
    return 1;
  }
  const auto filename = argv[1];
  const Mat image = imread(filename);
  if (image.empty())
  {
    cerr << "Could not read input image '" << filename << "'" << endl;
    return 2;
  }
  ShowImages(image);
  waitKey(0);
  return 0;
}
