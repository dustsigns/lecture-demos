//Illustration of image histograms
// Andreas Unterweger, 2017-2021
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
  
  const string window_name;
  
  histogram_data(const Mat &image, const string &window_name) 
   : image(image),
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

static double GetUsablePortionOfPlot(const Plot &plot, const double min_x, const double max_x)
{
  Point2d bottom_left, top_right;
  plot.GetVisibleRange(bottom_left, top_right);
  const auto min_visible_x = plot.GetVisibleXCoordinate(bottom_left.x);
  const auto max_visible_x = plot.GetVisibleXCoordinate(top_right.x);
  const auto min_usable_x = plot.GetVisibleXCoordinate(min_x);
  const auto max_usable_x = plot.GetVisibleXCoordinate(max_x);
  return static_cast<double>(max_usable_x - min_usable_x) / (max_visible_x - min_visible_x);
}

static Mat PlotHistograms(const histogram_data &data, const int number_of_bins)
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
    GetChannelHistogram(plane, number_of_bins, histogram);
    const auto bin_size = 256.0 / number_of_bins; //Size of each bin
    PointSet pointset(histogram, bin_size, color, false, false); //Don't interconnect points, but draw rectangles (samples without sample bars and a width proportional to the bin size) instead
    histograms.push_back(pointset);
  }
  Plot plot(histograms);
  plot.SetAxesLabels("I", "Freq.(I)");
  Tick::GenerateTicks(plot.x_axis_ticks, 0, 255, 10, 5); //Mark every 10 values, label every 50 (0-255)
  Mat_<Vec3b> image;
  plot.SetSmallBorders();
  const auto bin_size_pixels = static_cast<double>(data.image.rows) / number_of_bins;
  plot.DrawTo(image, data.image.cols, data.image.rows, [bin_size_pixels](Plot &plot)
                                                                        {
                                                                          const auto bin_size_factor = GetUsablePortionOfPlot(plot, 0, 255);
                                                                          const auto bin_size_reduced = static_cast<int>(ceil(bin_size_factor * bin_size_pixels)) - 1; //Leave extra space (for high numbers of bins)
                                                                          for_each(plot.point_sets.begin(), plot.point_sets.end(), [bin_size_reduced](PointSet &set) //Adjust bin size to usable range
                                                                                                                                                     {
                                                                                                                                                       set.line_width = bin_size_reduced;
                                                                                                                                                     });
                                                                        });
  return image;
}

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "Image and its histogram";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  const string trackbar_name = "Bins";
  static histogram_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  createTrackbar(trackbar_name, window_name, nullptr, 256, [](const int number_of_bins, void * const user_data)
                                                             {
                                                                 auto &data = *(static_cast<histogram_data*>(user_data));
                                                                 const Mat histogram_image = PlotHistograms(data, number_of_bins);
                                                                 const Mat combined_image = CombineImages({data.image, histogram_image}, Horizontal);
                                                                 imshow(data.window_name, combined_image);
                                                             }, static_cast<void*>(&data));
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
