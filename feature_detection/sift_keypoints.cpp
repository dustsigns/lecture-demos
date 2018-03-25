//Illustration of SIFT keypoints
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/xfeatures2d.hpp>

#include "combine.hpp"

using namespace std;

using namespace cv;
using namespace cv::xfeatures2d;

using namespace imgutils;

static void DetectFeatures(const Mat &image, vector<KeyPoint> &keypoints)
{
  auto feature_detector = SIFT::create();
  feature_detector->detect(image, keypoints);
}

static Mat DrawKeypoints(const Mat &image, const vector<KeyPoint> &keypoints)
{
  Mat image_with_keypoints;
  drawKeypoints(image, keypoints, image_with_keypoints, Scalar::all(-1), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
  return image_with_keypoints;
}

static Mat VisualizeKeypoints(const Mat &image)
{
  vector<KeyPoint> keypoints;
  DetectFeatures(image, keypoints);
  const Mat image_with_keypoints = DrawKeypoints(image, keypoints);
  return image_with_keypoints;
}

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "Image without and with keypoints";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  const Mat image_with_keypoints = VisualizeKeypoints(image);
  const Mat combined_image = CombineImages({image, image_with_keypoints}, Horizontal);
  imshow(window_name, combined_image);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the SIFT keypoints of an image" << endl;
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
