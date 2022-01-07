//Illustration of SIFT keypoint matching
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>

#include "combine.hpp"

struct match_data
{
  const cv::Mat first_image;
  const std::vector<cv::KeyPoint> first_keypoints;
  const cv::Mat second_image;
  const std::vector<cv::KeyPoint> second_keypoints;
  const std::vector<cv::DMatch> matches;
  
  const std::string window_name;
  
  match_data(const cv::Mat &first_image, const std::vector<cv::KeyPoint> &first_keypoints, const cv::Mat &second_image, const std::vector<cv::KeyPoint> &second_keypoints, const std::vector<cv::DMatch> &matches, const std::string &window_name)
   : first_image(first_image), first_keypoints(first_keypoints), second_image(second_image), second_keypoints(second_keypoints), matches(matches),
     window_name(window_name) {}
};

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
    assert(match.size() == 2);
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

static cv::Mat DrawMatches(const cv::Mat &first_image, const std::vector<cv::KeyPoint> &first_keypoints, const cv::Mat &second_image, const std::vector<cv::KeyPoint> &second_keypoints, const std::vector<cv::DMatch> &matches)
{
  constexpr auto line_width = 3;
  cv::Mat match_image;
  drawMatches(first_image, first_keypoints, second_image, second_keypoints, matches, match_image, line_width, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
  return match_image;
}

static cv::Mat VisualizeMatches(const match_data &data, const std::vector<cv::DMatch> &matches)
{
  const cv::Mat match_image = DrawMatches(data.first_image, data.first_keypoints, data.second_image, data.second_keypoints, matches);
  return match_image;
}

static void ShowImages(const cv::Mat &first_image, const cv::Mat &second_image)
{
  constexpr auto window_name = "First and second image";
  cv::namedWindow(window_name, cv::WINDOW_GUI_NORMAL | cv::WINDOW_AUTOSIZE);
  cv::moveWindow(window_name, 0, 0);
  std::vector<cv::KeyPoint> first_keypoints, second_keypoints;
  DetectFeatures(first_image, first_keypoints);
  DetectFeatures(second_image, second_keypoints);
  std::vector<cv::DMatch> matches;
  GetMatches(first_image, first_keypoints, second_image, second_keypoints, matches);
  assert(matches.size() > 0);
  static match_data data(first_image, first_keypoints, second_image, second_keypoints, matches, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  constexpr auto trackbar_name = "Match index";
  cv::createTrackbar(trackbar_name, window_name, nullptr, matches.size(),
                     [](const int visible_match, void * const user_data)
                       {
                         auto &data = *(static_cast<const match_data*>(user_data));
                         const cv::Mat original_images = imgutils::CombineImages({data.first_image, data.second_image}, imgutils::Horizontal);
                         assert(visible_match >= 0 && visible_match <= static_cast<int>(data.matches.size()));
                         const auto match_iterator = data.matches.begin() + visible_match - 1;
                         std::vector<cv::DMatch> single_match(match_iterator, match_iterator + 1);
                         const cv::Mat match_image = VisualizeMatches(data, single_match);
                         const cv::Mat combined_image = imgutils::CombineImages({original_images, match_image}, imgutils::Vertical);
                         cv::imshow(window_name, combined_image);
                       }, static_cast<void*>(&data));

  cv::setTrackbarPos(trackbar_name, window_name, matches.size() - 1); //Implies cv::imshow with last match
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    std::cout << "Illustrates how SIFT keypoints in two images can be matched." << std::endl;
    std::cout << "Usage: " << argv[0] << " <first image> <second image>" << std::endl;
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
  const cv::Mat second_image = cv::imread(second_image_filename);
  if (second_image.empty())
  {
    std::cerr << "Could not read second image '" << second_image_filename << "'" << std::endl;
    return 3;
  }
  ShowImages(first_image, second_image);
  cv::waitKey(0);
  return 0;
}
