//Illustration of SIFT matching for finding a perspective transform
// Andreas Unterweger, 2019-2025
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include "colors.hpp"
#include "combine.hpp"
#include "window.hpp"

static int OpenVideo(const char * const filename, cv::VideoCapture &capture)
{
  using namespace std::string_literals;
  const bool use_webcam = "-"s == filename; //Interpret - as the default webcam
  const bool opened = use_webcam ? capture.open(0) : capture.open(filename);
  if (use_webcam) //Minimize buffering for webcams to return up-to-date images
    capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
  return opened;
}

static void DetectFeatures(const cv::Mat &image, std::vector<cv::KeyPoint> &keypoints)
{
  auto feature_detector = cv::SIFT::create();
  feature_detector->detect(image, keypoints);
}

static void ExtractDescriptors(const cv::Mat &image, std::vector<cv::KeyPoint> &keypoints, cv::Mat &descriptors)
{
  auto extractor = cv::SIFT::create();
  extractor->compute(image, keypoints, descriptors);
}

static void FilterMatches(const std::vector<std::vector<cv::DMatch>> matches, std::vector<cv::DMatch> &filtered_matches)
{
  const double second_to_first_match_distance_ratio = 0.8; //From Lowe's 2004 paper
  for (const auto &match : matches)
  {
    if (match.size() < 2)
      continue;
    if (match[0].distance < second_to_first_match_distance_ratio * match[1].distance) //Only keep "good" matches, i.e., those where the second candidate is significantly worse than the first
      filtered_matches.push_back(match[0]);
  }
}

static void MatchFeatures(const cv::Mat &first_descriptors, const cv::Mat &second_descriptors, std::vector<cv::DMatch> &filtered_matches)
{
  cv::BFMatcher matcher;
  std::vector<std::vector<cv::DMatch>> matches;
  matcher.knnMatch(first_descriptors, second_descriptors, matches, 2); //Find the two best matches
  FilterMatches(matches, filtered_matches);
}

static void GetMatches(const cv::Mat &first_image, std::vector<cv::KeyPoint> &first_keypoints, const cv::Mat &second_image, std::vector<cv::KeyPoint> &second_keypoints, std::vector<cv::DMatch> &matches)
{
  cv::Mat first_descriptors, second_descriptors;
  ExtractDescriptors(first_image, first_keypoints, first_descriptors);
  ExtractDescriptors(second_image, second_keypoints, second_descriptors);
  MatchFeatures(first_descriptors, second_descriptors, matches);
}

static void GetMatchingKeyPoints(const std::vector<cv::DMatch> &matches, const std::vector<cv::KeyPoint> &first_keypoints, const std::vector<cv::KeyPoint> &second_keypoints, std::vector<cv::Point2f> &first_image_points, std::vector<cv::Point2f> &second_image_points)
{
  for (const auto &match : matches)
  {
    first_image_points.push_back(first_keypoints[match.queryIdx].pt);
    second_image_points.push_back(second_keypoints[match.trainIdx].pt);
  }
}

static bool FindHomography(const cv::Mat &first_image, const cv::Mat &second_image, cv::Mat &homography)
{
  std::vector<cv::KeyPoint> first_keypoints, second_keypoints;
  DetectFeatures(first_image, first_keypoints);
  DetectFeatures(second_image, second_keypoints);
  std::vector<cv::DMatch> matches;
  GetMatches(first_image, first_keypoints, second_image, second_keypoints, matches);
  if (matches.size() == 0)
    return false;
  std::vector<cv::Point2f> first_image_points, second_image_points;
  GetMatchingKeyPoints(matches, first_keypoints, second_keypoints, first_image_points, second_image_points);
  homography = cv::findHomography(first_image_points, second_image_points, cv::RANSAC);
  return !homography.empty();
}

static void TransformImageRectangle(const cv::Size2i original_size, const cv::Mat &homography, std::vector<cv::Point2i> &transformed_corners)
{
  const cv::Point2f top_left(0, 0);
  const cv::Point2f bottom_left(original_size.width, 0);
  const cv::Point2f bottom_right(original_size.width, original_size.height);
  const cv::Point2f top_right(0, original_size.height);
  const std::vector<cv::Point2f> original_corners { top_left, bottom_left, bottom_right, top_right };
  std::vector<cv::Point2f> transformed_corners_float;
  perspectiveTransform(original_corners, transformed_corners_float, homography);
  assert(transformed_corners.empty());
  std::transform(transformed_corners_float.begin(), transformed_corners_float.end(), back_inserter(transformed_corners),
                 [](const cv::Point2f &p)
                   {
                     return cv::Point2i(p);
                   });
}
 
static void DrawImageRectangle(cv::Mat image, const std::vector<cv::Point2i> &transformed_corners)
{
  constexpr auto line_width = 2;
  const auto frame_color = imgutils::Red;
  cv::polylines(image, transformed_corners, true, frame_color, line_width); //Closed polylines
}

static void TransformAndDrawImageRectangle(const cv::Mat &first_image, cv::Mat &second_image, const cv::Mat &homography)
{
  std::vector<cv::Point2i> transformed_corners;
  TransformImageRectangle(first_image.size(), homography, transformed_corners);
  DrawImageRectangle(second_image, transformed_corners);
}

static void ShowImage(const cv::Mat &first_image, cv::Mat &second_image, imgutils::Window &window)
{
  cv::Mat homography;
  const bool found = FindHomography(first_image, second_image, homography);
  if (found)
    TransformAndDrawImageRectangle(first_image, second_image, homography);
  const cv::Mat combined_image = imgutils::CombineImages({first_image, second_image}, imgutils::CombinationMode::Horizontal);
  window.UpdateContent(combined_image);
}

static void ShowImages(cv::VideoCapture &capture, const cv::Mat &first_image, const int wait_time)
{
  constexpr auto window_name = "Original and found (perspective-transformed) image";
  imgutils::Window window(window_name);

  cv::Mat second_image;
  while (capture.read(second_image) && !second_image.empty())
  {
    ShowImage(first_image, second_image, window);
    if (window.ShowInteractive(nullptr, wait_time, false) == 'q') //Do not hide window after each image; interpret Q key press as exit
      break;
  }
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3 && argc != 4)
  {
    std::cout << "Illustrates how to find an image with a perspective transform within another image." << std::endl;
    std::cout << "Usage: " << argv[0] << " <first image> <second image or video of second images> [<wait time between second images>]" << std::endl;
    return 1;
  }
  const auto first_image_filename = argv[1];
  const cv::Mat first_image = cv::imread(first_image_filename);
  if (first_image.empty())
  {
    std::cerr << "Could not read first image '" << first_image_filename << "'" << std::endl;
    return 2;
  }
  const auto second_image_filename = argv[2];
  int wait_time = 0;
  if (argc == 4)
  {
    const auto wait_time_text = argv[3];
    wait_time = std::stoi(wait_time_text);
  }
  cv::VideoCapture capture;
  if (!OpenVideo(second_image_filename, capture))
  {
    std::cerr << "Could not open second image '" << second_image_filename << "'" << std::endl;
    return 3;
  }
  ShowImages(capture, first_image, wait_time);
  return 0;
}
