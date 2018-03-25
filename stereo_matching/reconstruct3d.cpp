//Illustration of 3-D reconstruction from a disparity image
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <cmath>

#include "opencv2/core.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/viz.hpp"

using namespace std;

using namespace cv;
using namespace cv::viz;

static void FilterDepthImage(Mat &depth_image)
{
  const auto filter_threshold = depth_image.rows;
  constexpr auto NaN = numeric_limits<float>::quiet_NaN();
  const Vec3f NaN_pixel = Vec3f(NaN);
  
  auto filtered_depth_image = static_cast<Mat_<Vec3f>>(depth_image);
  for (auto &pixel : filtered_depth_image)
  {
    if (pixel[2] > filter_threshold)
      pixel = NaN_pixel;
  }
}

static Mat DisparityImageTo3D(const Mat &disparity_image)
{
  const auto Q = Mat::eye(4, 4, CV_64FC1); //Use identity matrix as Q for simplicity (z coordinate = 1 / disparity)
  Mat depth_image;
  reprojectImageTo3D(disparity_image, depth_image, Q, true);
  FilterDepthImage(depth_image);
  return depth_image;
}

static void ShowWindow(const Mat &disparity_image, const Mat &left_image)
{
  Viz3d visualization("3-D reconstruction");
  const Mat depth_image = DisparityImageTo3D(disparity_image);
  WCloud reconstruction(depth_image, left_image);
  visualization.showWidget("Point cloud", reconstruction);
  /*visualization.spinOnce(1, true); //TODO: Why does zooming no longer work once the camera has been moved? [fixed in new OpenCV version > 3.4.0, but buggy in 3.4.1; update as soon as > 3.4.1 it is available]
  auto pose = visualization.getViewerPose();
  pose = pose.rotate(Vec3f(M_PI, 0, 0)); //Flip camera around so that it faces into the right direction
  visualization.setViewerPose(pose);*/
  visualization.spinOnce(1, true);
  while (!visualization.wasStopped())
    visualization.spinOnce(1, true);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    cout << "Illustrates 3-D reconstruction from an image and its disparity image." << endl;
    cout << "Usage: " << argv[0] << " <left image> <disparity image>" << endl;
    return 1;
  }
  const auto left_image_path = argv[1];
  const Mat left_image = imread(left_image_path);
  if (left_image.empty())
  {
    cerr << "Could not read left image '" << left_image_path << "'" << endl;
    return 2;
  }
  const auto disparity_image_path = argv[2];
  const Mat disparity_image = imread(disparity_image_path, IMREAD_GRAYSCALE);
  if (disparity_image.empty())
  {
    cerr << "Could not read disparity image '" << disparity_image_path << "'" << endl;
    return 3;
  }
  ShowWindow(disparity_image, left_image);
  return 0;
}
