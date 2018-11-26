//Illustration of panoramic images
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/xfeatures2d.hpp>
#include "opencv2/calib3d.hpp"

#include "combine.hpp"

using namespace std;

using namespace cv;
using namespace cv::xfeatures2d;

using namespace imgutils;

static void ExtractDescriptors(const Mat &image, vector<KeyPoint> &keypoints, Mat &descriptors)
{
  auto feature_detector = SIFT::create();
  feature_detector->detect(image, keypoints);
  feature_detector->compute(image, keypoints, descriptors);
}

static void FilterMatches(const vector<vector<DMatch>> matches, vector<DMatch> &filtered_matches)
{
  const double second_to_first_match_distance_ratio = 0.8; //From Lowe's 2004 paper
  for (const auto &match : matches)
  {
    assert(match.size() == 2);
    if (match[0].distance < second_to_first_match_distance_ratio * match[1].distance) //Only keep "good" matches, i.e., those where the second candidate is significantly worse than the first
      filtered_matches.push_back(match[0]);
  }
}

static void MatchFeatures(const Mat &first_descriptors, const Mat &second_descriptors, vector<DMatch> &filtered_matches)
{
  BFMatcher matcher;
  vector<vector<DMatch>> matches;
  matcher.knnMatch(first_descriptors, second_descriptors, matches, 2); //Find the two best matches
  FilterMatches(matches, filtered_matches);
}

static void GetMatches(const Mat &first_image, vector<KeyPoint> &first_keypoints, const Mat &second_image, vector<KeyPoint> &second_keypoints, vector<DMatch> &matches)
{
  Mat first_descriptors, second_descriptors;
  ExtractDescriptors(first_image, first_keypoints, first_descriptors);
  ExtractDescriptors(second_image, second_keypoints, second_descriptors);
  MatchFeatures(first_descriptors, second_descriptors, matches);
}

static void ExtractPoints(const vector<KeyPoint> &first_keypoints, const vector<KeyPoint> &second_keypoints, const vector<DMatch> matches, vector<Point2f> &first_points, vector<Point2f> &second_points)
{
  for (const auto& match : matches)
  {
    first_points.push_back(first_keypoints[match.queryIdx].pt);
    second_points.push_back(second_keypoints[match.trainIdx].pt);
  }
}

static Mat StitchImages(const Mat &first_image, const vector<Point2f> &first_points, const Mat &second_image, const vector<Point2f> &second_points)
{
  Mat stitched_image;
  Mat homography = findHomography(second_points, first_points, RANSAC);
  warpPerspective(second_image, stitched_image, homography, Size(first_image.cols + second_image.cols, MAX(first_image.rows, second_image.rows)));
  first_image.copyTo(stitched_image(Rect(0, 0, first_image.cols, first_image.rows)));
  return stitched_image;
}

static Mat MakePanoramicImage(const Mat &first_image, const Mat &second_image)
{
  vector<KeyPoint> first_keypoints, second_keypoints;
  vector<DMatch> matches;
  GetMatches(first_image, first_keypoints, second_image, second_keypoints, matches);
  vector<Point2f> first_points, second_points;
  ExtractPoints(first_keypoints, second_keypoints, matches, first_points, second_points);
  const Mat stitched_image = StitchImages(first_image, first_points, second_image, second_points);
  return stitched_image;
}

static void ShowImages(const Mat &first_image, const Mat &second_image)
{
  constexpr auto window_name = "First and second image combined";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  const Mat original_images = CombineImages({first_image, second_image}, Horizontal);
  const Mat panoramic_image = MakePanoramicImage(first_image, second_image);
  const Mat combined_image = CombineImages({original_images, panoramic_image}, Vertical);
  imshow(window_name, combined_image);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    cout << "Illustrates how a panoramic image can be created from SIFT keypoint matches" << endl;
    cout << "Usage: " << argv[0] << " <first image> <second image>" << endl; //TODO: Allow for more images
    return 1;
  }
  const auto first_image_filename = argv[1];
  const Mat first_image = imread(first_image_filename);
  if (first_image.empty())
  {
    cerr << "Could not read first image '" << first_image_filename << "'" << endl;
    return 2;
  }
  const auto second_image_filename = argv[2];
  const Mat second_image = imread(second_image_filename);
  if (second_image.empty())
  {
    cerr << "Could not read second image '" << second_image_filename << "'" << endl;
    return 2;
  }
  ShowImages(first_image, second_image);
  waitKey(0);
  return 0;
}
