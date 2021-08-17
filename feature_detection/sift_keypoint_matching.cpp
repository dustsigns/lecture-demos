//Illustration of SIFT keypoint matching
// Andreas Unterweger, 2017-2021
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>

#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

struct match_data
{
  const Mat first_image;
  const vector<KeyPoint> first_keypoints;
  const Mat second_image;
  const vector<KeyPoint> second_keypoints;
  const vector<DMatch> matches;
  
  const string window_name;
  
  match_data(const Mat &first_image, const vector<KeyPoint> &first_keypoints, const Mat &second_image, const vector<KeyPoint> &second_keypoints, const vector<DMatch> &matches, const string &window_name)
   : first_image(first_image), first_keypoints(first_keypoints), second_image(second_image), second_keypoints(second_keypoints), matches(matches),
     window_name(window_name) {}
};

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

static Mat DrawMatches(const Mat &first_image, const vector<KeyPoint> &first_keypoints, const Mat &second_image, const vector<KeyPoint> &second_keypoints, const vector<DMatch> &matches)
{
  constexpr auto line_width = 3;
  Mat match_image;
  drawMatches(first_image, first_keypoints, second_image, second_keypoints, matches, match_image, line_width, Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
  return match_image;
}

static Mat VisualizeMatches(const match_data &data, const vector<DMatch> &matches)
{
  const Mat match_image = DrawMatches(data.first_image, data.first_keypoints, data.second_image, data.second_keypoints, matches);
  return match_image;
}

static void ShowImages(const Mat &first_image, const Mat &second_image)
{
  constexpr auto window_name = "First and second image";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  vector<KeyPoint> first_keypoints, second_keypoints;
  DetectFeatures(first_image, first_keypoints);
  DetectFeatures(second_image, second_keypoints);
  vector<DMatch> matches;
  GetMatches(first_image, first_keypoints, second_image, second_keypoints, matches);
  assert(matches.size() > 0);
  static match_data data(first_image, first_keypoints, second_image, second_keypoints, matches, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  constexpr auto trackbar_name = "Match index";
  createTrackbar(trackbar_name, window_name, nullptr, matches.size(), [](const int visible_match, void * const user_data)
                                                                        {
                                                                          auto &data = *(static_cast<const match_data*>(user_data));
                                                                          const Mat original_images = CombineImages({data.first_image, data.second_image}, Horizontal);
                                                                          assert(visible_match >= 0 && visible_match <= static_cast<int>(data.matches.size()));
                                                                          const auto match_iterator = data.matches.begin() + visible_match - 1;
                                                                          vector<DMatch> single_match(match_iterator, match_iterator + 1);
                                                                          const Mat match_image = VisualizeMatches(data, single_match);
                                                                          const Mat combined_image = CombineImages({original_images, match_image}, Vertical);
                                                                          imshow(window_name, combined_image);
                                                                        }, static_cast<void*>(&data));

  setTrackbarPos(trackbar_name, window_name, matches.size() - 1); //Implies imshow with last match
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    cout << "Illustrates how SIFT keypoints in two images can be matched." << endl;
    cout << "Usage: " << argv[0] << " <first image> <second image>" << endl;
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
    return 3;
  }
  ShowImages(first_image, second_image);
  waitKey(0);
  return 0;
}
