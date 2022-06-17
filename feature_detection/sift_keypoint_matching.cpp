//Illustration of SIFT keypoint matching
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>
#include <memory>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>

#include "combine.hpp"
#include "window.hpp"

class match_data
{
  protected:
    imgutils::Window window;

    using TrackBarType = imgutils::TrackBar<match_data&>;
    std::unique_ptr<TrackBarType> match_trackbar;
    
    const cv::Mat first_image;
    const cv::Mat second_image;
    
    std::vector<cv::KeyPoint> first_keypoints;
    std::vector<cv::KeyPoint> second_keypoints;
    std::vector<cv::DMatch> matches;
  
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

    void GetMatches()
    {
      cv::Mat first_descriptors, second_descriptors;
      ExtractDescriptors(first_image, first_keypoints, first_descriptors);
      ExtractDescriptors(second_image, second_keypoints, second_descriptors);
      MatchFeatures(first_descriptors, second_descriptors, matches);
    }
    
    cv::Mat DrawMatches(const std::vector<cv::DMatch> &matches)
    {
      constexpr auto line_width = 3;
      cv::Mat match_image;
      drawMatches(first_image, first_keypoints, second_image, second_keypoints, matches, match_image, line_width, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
      return match_image;
    }

    cv::Mat VisualizeMatches(const int visible_match_index)
    {
      assert(visible_match_index >= 0 && visible_match_index <= static_cast<int>(matches.size()));
      const auto match_iterator = matches.begin() + visible_match_index;
      std::vector<cv::DMatch> single_match(match_iterator, match_iterator + 1);
      const cv::Mat match_image = DrawMatches(single_match);
      return match_image;
    }
    
    static void UpdateImage(match_data &data)
    {
       const cv::Mat original_images = imgutils::CombineImages({data.first_image, data.second_image}, imgutils::CombinationMode::Horizontal);
       const int visible_match_index = data.match_trackbar->GetValue();
       const cv::Mat match_image = data.VisualizeMatches(visible_match_index);
       const cv::Mat combined_image = imgutils::CombineImages({original_images, match_image}, imgutils::CombinationMode::Vertical);
       data.window.UpdateContent(combined_image);
    }
  
    static constexpr auto window_name = "First and second image";
    static constexpr auto match_trackbar_name = "Match index";
  public:
    match_data(const cv::Mat &first_image, const cv::Mat &second_image)
     : window(window_name),
       first_image(first_image), second_image(second_image)
    {
      DetectFeatures(first_image, first_keypoints);
      DetectFeatures(second_image, second_keypoints);
      GetMatches();
      const auto number_of_matches = matches.size();
      assert(number_of_matches > 0);
      match_trackbar = std::make_unique<TrackBarType>(match_trackbar_name, window, number_of_matches - 1, 0, number_of_matches - 1, UpdateImage, *this); //Last match by default
      
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImage()
    {
      window.ShowInteractive();
    }
};

static void ShowImage(const cv::Mat &first_image, const cv::Mat &second_image)
{
  match_data data(first_image, second_image);
  data.ShowImage();
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
  ShowImage(first_image, second_image);
  return 0;
}
