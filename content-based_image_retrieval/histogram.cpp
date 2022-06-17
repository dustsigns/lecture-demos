//Illustration of image histograms
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "math.hpp"
#include "plot.hpp"
#include "colors.hpp"
#include "format.hpp"
#include "combine.hpp"
#include "window.hpp"

class histogram_data
{
  protected:
    imgutils::Window window;
    
    using TrackBarType = imgutils::TrackBar<histogram_data&>;
    TrackBarType bin_trackbar;
  
    const cv::Mat image;

    static void ComputeRelativeHistogram(const cv::Mat_<float> &histogram, std::vector<float> &relative_histogram)
    {
      assert(histogram.channels() == 1);
      const auto element_sum = sum(histogram)[0];
      relative_histogram.resize(histogram.total());
      std::transform(histogram.begin(), histogram.end(), relative_histogram.begin(), [element_sum](const float &value)
                                                                                                  {
                                                                                                    return 100.f * value / element_sum;
                                                                                                  });
    }

    static void GetChannelHistogram(const cv::Mat &image, const int number_of_bins, std::vector<float> &histogram)
    {
      assert(image.type() == CV_8UC1);
      assert(number_of_bins > 0);
      constexpr int channels[] { 0 };
      const int bins[] { number_of_bins };
      constexpr float channel_range[] { 0, 256 }; //Upper bound is exclusive
      const float * /*const*/ ranges[] { channel_range }; //Should be const, but calcHist requires it to be non-const
      cv::Mat histogram_matrix;
      calcHist(&image, 1, channels, cv::Mat(), histogram_matrix, 1, bins, ranges); //1-D histogram
      ComputeRelativeHistogram(histogram_matrix, histogram);
    }

    static double GetUsablePortionOfPlot(const imgutils::Plot &plot, const double min_x, const double max_x)
    {
      cv::Point2d bottom_left, top_right;
      plot.GetVisibleRange(bottom_left, top_right);
      const auto min_visible_x = plot.GetVisibleXCoordinate(bottom_left.x);
      const auto max_visible_x = plot.GetVisibleXCoordinate(top_right.x);
      const auto min_usable_x = plot.GetVisibleXCoordinate(min_x);
      const auto max_usable_x = plot.GetVisibleXCoordinate(max_x);
      return static_cast<double>(max_usable_x - min_usable_x) / (max_visible_x - min_visible_x);
    }

    cv::Mat PlotHistograms()
    {
      const int number_of_bins = bin_trackbar.GetValue();
      
      cv::Mat bgr_planes[3];
      split(image, bgr_planes);
      const std::vector<std::pair<const cv::Mat, const cv::Vec3b>> planes_and_colors{{bgr_planes[0], imgutils::Blue},
                                                                                     {bgr_planes[1], imgutils::Green},
                                                                                     {bgr_planes[2], imgutils::Red}};
      std::vector<imgutils::PointSet> histograms;
      for (const auto &plane_and_color : planes_and_colors)
      {
        const auto &plane = plane_and_color.first;
        const auto &color = plane_and_color.second;
        std::vector<float> histogram;
        GetChannelHistogram(plane, number_of_bins, histogram);
        const auto bin_size = 256.0 / number_of_bins; //Size of each bin
        imgutils::PointSet pointset(histogram, bin_size, color, false, false); //Don't interconnect points, but draw rectangles (samples without sample bars and a width proportional to the bin size) instead
        histograms.push_back(pointset);
      }
      imgutils::Plot plot(histograms);
      plot.SetAxesLabels("I", "Freq.(I)");
      imgutils::Tick::GenerateTicks(plot.x_axis_ticks, 0, 255, 10, 5); //Mark every 10 values, label every 50 (0-255)
      cv::Mat_<cv::Vec3b> output_image;
      plot.SetSmallBorders();
      const auto overestimated_bin_size_pixels = static_cast<double>(image.cols) / number_of_bins;
      plot.DrawTo(output_image, image.cols, image.rows,
                  [overestimated_bin_size_pixels](imgutils::Plot &plot)
                                                 {
                                                   const auto overestimation_factor = GetUsablePortionOfPlot(plot, 0, 255);
                                                   const auto bin_size_pixels = static_cast<int>(round(overestimation_factor * overestimated_bin_size_pixels)) - 1; //Leave extra space (for high numbers of bins)
                                                   std::for_each(plot.point_sets.begin(), plot.point_sets.end(), [bin_size_pixels](imgutils::PointSet &set) //Adjust bin size to usable range
                                                                                                                                   {
                                                                                                                                     set.line_width = bin_size_pixels;
                                                                                                                                   });
                                                 });
      return output_image;
    }

    static void UpdateImage(histogram_data &data)
    {
      const cv::Mat histogram_image = data.PlotHistograms();
      const cv::Mat combined_image = imgutils::CombineImages({data.image, histogram_image}, imgutils::CombinationMode::Horizontal);
      data.window.UpdateContent(combined_image);
    }

    static constexpr auto window_name = "Image and its histogram";
    static constexpr auto bin_trackbar_name = "Bins";
  public:
    histogram_data(const cv::Mat &image) 
     : window(window_name),
       bin_trackbar(bin_trackbar_name, window, 256, 2, 256, UpdateImage, *this), //256 bins by default (minimum 2 as a histogram with less than two bins does not make sense)
       image(image)
    {
      assert(image.type() == CV_8UC3); //UpdateImage assumes RGB images
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage(const cv::Mat &image)
{
  histogram_data data(image);
  data.ShowImage();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the different histograms of an RGB image." << std::endl;
    std::cout << "Usage: " << argv[0] << " <input image>" << std::endl;
    return 1;
  }
  const auto image_filename = argv[1];
  const cv::Mat image = cv::imread(image_filename);
  if (image.empty())
  {
    std::cerr << "Could not read input image '" << image_filename << "'" << std::endl;
    return 2;
  }
  ShowImage(image);
  return 0;
}
