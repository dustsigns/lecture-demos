//Illustration of SIFT matching for finding a perspective transform
// Andreas Unterweger, 2019
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include "colors.hpp"
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

static void ExtractDescriptors(const Mat &image, vector<KeyPoint> &keypoints, Mat &descriptors)
{
  auto extractor = SIFT::create();
  extractor->compute(image, keypoints, descriptors);
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

static void GetMatchingKeyPoints(const vector<DMatch> &matches, const vector<KeyPoint> &first_keypoints, const vector<KeyPoint> &second_keypoints, vector<Point2f> &first_image_points, vector<Point2f> &second_image_points)
{
  for (const auto &match : matches)
  {
    first_image_points.push_back(first_keypoints[match.queryIdx].pt);
    second_image_points.push_back(second_keypoints[match.trainIdx].pt);
  }
}

static bool FindHomography(const Mat &first_image, const Mat &second_image, Mat &homography)
{
  vector<KeyPoint> first_keypoints, second_keypoints;
  DetectFeatures(first_image, first_keypoints);
  DetectFeatures(second_image, second_keypoints);
  vector<DMatch> matches;
  GetMatches(first_image, first_keypoints, second_image, second_keypoints, matches);
  if (matches.size() == 0)
    return false;
  vector<Point2f> first_image_points, second_image_points;
  GetMatchingKeyPoints(matches, first_keypoints, second_keypoints, first_image_points, second_image_points);
  homography = findHomography(first_image_points, second_image_points, RANSAC);
  return !homography.empty();
}

static void TransformImageRectangle(const Size2i original_size, const Mat &homography, vector<Point2i> &transformed_corners)
{
  const Point2f top_left(0, 0);
  const Point2f bottom_left(original_size.width, 0);
  const Point2f bottom_right(original_size.width, original_size.height);
  const Point2f top_right(0, original_size.height);
  const vector<Point2f> original_corners { top_left, bottom_left, bottom_right, top_right };
  vector<Point2f> transformed_corners_float;
  perspectiveTransform(original_corners, transformed_corners_float, homography);
  assert(transformed_corners.empty());
  transform(transformed_corners_float.begin(), transformed_corners_float.end(), back_inserter(transformed_corners),
            [](const Point2f &p)
              {
                return Point2i(p);
              });
}
 
static void DrawImageRectangle(Mat image, const vector<Point2i> &transformed_corners)
{
  constexpr auto line_width = 3;
  polylines(image, transformed_corners, true, Red, line_width); //Closed polylines
}

static void TransformAndDrawImageRectangle(const Mat &first_image, Mat &second_image, const Mat &homography)
{
  vector<Point2i> transformed_corners;
  TransformImageRectangle(first_image.size(), homography, transformed_corners);
  DrawImageRectangle(second_image, transformed_corners);
}

static void ShowImages(const Mat &first_image, Mat &second_image)
{
  constexpr auto window_name = "Original and found (perspective-transformed) image";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  Mat homography;
  const bool found = FindHomography(first_image, second_image, homography);
  if (found)
    TransformAndDrawImageRectangle(first_image, second_image, homography);
  const Mat combined_image = CombineImages({first_image, second_image}, Horizontal);
  imshow(window_name, combined_image);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    cout << "Illustrates how to find an image with a perspective transform within another image." << endl;
    cout << "Usage: " << argv[0] << " <first image> <second image or video of second images>" << endl;
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
  VideoCapture capture(second_image_filename);
  if (!capture.isOpened())
  {
    cerr << "Could not open second image '" << second_image_filename << "'" << endl;
    return 3;
  }
  Mat second_image;
  while (capture.read(second_image) && !second_image.empty())
  {
    ShowImages(first_image, second_image);
    waitKey(0);
  }
  return 0;
}
