//Illustration of image histograms
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "math.hpp"
#include "plot.hpp"
#include "colors.hpp"
#include "format.hpp"
#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

struct histogram_data
{
  const Mat image;
  int number_of_bins;
  
  const string window_name;
  
  histogram_data(const Mat &image, const string &window_name) 
   : image(image),
     number_of_bins(64),
     window_name(window_name) { }
};

static void ComputeRelativeHistogram(const Mat_<float> &histogram, vector<float> &relative_histogram)
{
  assert(histogram.channels() == 1);
  const auto element_sum = sum(histogram)[0];
  relative_histogram.resize(histogram.total());
  transform(histogram.begin(), histogram.end(), relative_histogram.begin(), [element_sum](const float &value)
                                                                                         {
                                                                                           return 100.f * value / element_sum;
                                                                                         });
}

static void GetChannelHistogram(const Mat &image, const int number_of_bins, vector<float> &histogram)
{
  assert(image.type() == CV_8UC1);
  assert(number_of_bins > 0);
  constexpr int channels[] { 0 };
  const int bins[] { number_of_bins };
  constexpr float channel_range[] { 0, 256 }; //Upper bound is exclusive
  const float * /*const*/ ranges[] { channel_range }; //Should be const, but calcHist requires it to be non-const
  Mat histogram_matrix;
  calcHist(&image, 1, channels, Mat(), histogram_matrix, 1, bins, ranges); //1-D histogram
  ComputeRelativeHistogram(histogram_matrix, histogram);
}

static Mat PlotHistograms(const histogram_data &data)
{
  assert(data.image.type() == CV_8UC3);
  Mat bgr_planes[3];
  split(data.image, bgr_planes);
  const vector<pair<const Mat, const Vec3b>> planes_and_colors{{bgr_planes[0], Blue},
                                                               {bgr_planes[1], Green},
                                                               {bgr_planes[2], Red}};
  vector<PointSet> histograms;
  for (const auto &plane_and_color : planes_and_colors)
  {
    const auto &plane = plane_and_color.first;
    const auto &color = plane_and_color.second;
    vector<float> histogram;
    GetChannelHistogram(plane, data.number_of_bins, histogram);
    const auto bin_size = 256.0 / data.number_of_bins; //Size of each bin
    PointSet pointset(histogram, bin_size, color, false, true, false, data.image.rows / data.number_of_bins - 1); //Don't interconnect points, but draw samples without sample bars instead
    histograms.push_back(pointset);
  }
  Plot plot(histograms);
  plot.SetAxesLabels("Val.", "Freq.");
  Tick::GenerateTicks(plot.x_axis_ticks, 0, 255, 10, 5); //Mark every 10 values, label every 50 (0-255)
  Mat_<Vec3b> image;
  plot.DrawTo(image, data.image.cols, data.image.rows); //TODO: Fix crash for small values (<=10) of number_of_bins
  return image;
}

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "Image and its histogram";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  const string trackbar_name = "Bins";
  static histogram_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  createTrackbar(trackbar_name, window_name, &data.number_of_bins, 256, [](const int, void * const user_data)
                                                                          {
                                                                              auto &data = *((histogram_data * const)user_data);
                                                                              const Mat histogram_image = PlotHistograms(data);
                                                                              const Mat combined_image = CombineImages({data.image, histogram_image}, Horizontal);
                                                                              imshow(data.window_name, combined_image);
                                                                          }, (void*)&data);
  setTrackbarMin(trackbar_name, window_name, 2); //A histogram with less than two bins does not make sense
  setTrackbarPos(trackbar_name, window_name, 256); //Implies imshow with 256 bins
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the different histograms of an RGB image." << endl;
    cout << "Usage: " << argv[0] << " <input image>" << endl;
    return 1;
  }
  const auto image_filename = argv[1];
  const Mat image = imread(image_filename);
  if (image.empty())
  {
    cerr << "Could not read input image '" << image_filename << "'" << endl;
    return 2;
  }
  ShowImages(image);
  waitKey(0);
  return 0;
}
