//Illustration of 3-D reconstruction from a disparity image
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <cmath>

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/viz.hpp>

#include "vizwin.hpp"

static void FilterDepthImage(cv::Mat &depth_image)
{
  const auto filter_threshold = depth_image.rows;
  constexpr auto NaN = std::numeric_limits<float>::quiet_NaN();
  const cv::Vec3f NaN_pixel = cv::Vec3f(NaN);
  
  auto filtered_depth_image = static_cast<cv::Mat_<cv::Vec3f>>(depth_image);
  std::replace_if(filtered_depth_image.begin(), filtered_depth_image.end(),
                  [&filter_threshold](const auto &pixel)
                                     { 
                                       return pixel[2] > filter_threshold;
                                     }, NaN_pixel);
}

static cv::Mat DisparityImageTo3D(const cv::Mat &disparity_image)
{
  const auto Q = cv::Mat::eye(4, 4, CV_64FC1); //Use identity matrix as Q for simplicity (z coordinate = 1 / disparity)
  cv::Mat depth_image;
  cv::reprojectImageTo3D(disparity_image, depth_image, Q, true);
  FilterDepthImage(depth_image);
  return depth_image;
}

static void ShowWindow(const cv::Mat &disparity_image, const cv::Mat &left_image)
{
  constexpr auto window_name = "3-D reconstruction";
  vizutils::VisualizationWindow window(window_name);
  const cv::Mat depth_image = DisparityImageTo3D(disparity_image);
  cv::viz::WCloud reconstruction(depth_image, left_image);
  window.AddWidget("Point cloud", &reconstruction);
  window.ShowInteractive([&window]()
                                  {
                                    //TODO: Why does zooming no longer work once the pose is read and set again (even without changes)
                                    /*auto pose = window.GetViewerPose();
                                    pose = pose.rotate(cv::Vec3f(M_PI, 0, 0)); //Flip camera around so that it faces into the right direction
                                    window.SetViewerPose(pose);*/
                                  });
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    std::cout << "Illustrates 3-D reconstruction from an image and its disparity image." << std::endl;
    std::cout << "Usage: " << argv[0] << " <left image> <disparity image>" << std::endl;
    return 1;
  }
  const auto left_image_path = argv[1];
  const cv::Mat left_image = cv::imread(left_image_path);
  if (left_image.empty())
  {
    std::cerr << "Could not read left image '" << left_image_path << "'" << std::endl;
    return 2;
  }
  const auto disparity_image_path = argv[2];
  const cv::Mat disparity_image = cv::imread(disparity_image_path, cv::IMREAD_GRAYSCALE);
  if (disparity_image.empty())
  {
    std::cerr << "Could not read disparity image '" << disparity_image_path << "'" << std::endl;
    return 3;
  }
  ShowWindow(disparity_image, left_image);
  return 0;
}
