//Helper functions for drawing aligned text
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <type_traits>

#include <opencv2/imgproc.hpp>

#include "drawtext.hpp"

namespace imgutils
{
  static bool AlignmentContains(TextAlignment alignment, TextAlignment contained_alignment)
  {
    const auto all = static_cast<std::underlying_type<TextAlignment>::type>(alignment);
    const auto contained = static_cast<std::underlying_type<TextAlignment>::type>(contained_alignment);
    const auto result = (all & contained) == contained;
    return result;
  }
  
  void DrawText(cv::Mat_<cv::Vec3b> &image, const std::string &text, const cv::Point &point, const TextAlignment alignment, const cv::Vec3b &color, const int cv_font_face, const double cv_font_scale)
  {
    int baseline;
    const auto text_size = cv::getTextSize(text, cv_font_face, cv_font_scale, 1, &baseline);
    auto offset_point = point; //Default is top-left (no offset)
    
    if (AlignmentContains(alignment, TextAlignment::Center))
      offset_point.x -= text_size.width / 2;
    if (AlignmentContains(alignment, TextAlignment::Right))
      offset_point.x -= text_size.width;
    if (AlignmentContains(alignment, TextAlignment::Middle))
      offset_point.y += text_size.height / 2;
    if (AlignmentContains(alignment, TextAlignment::Bottom))
      offset_point.y += text_size.height;
    cv::putText(image, text, offset_point, cv_font_face, cv_font_scale, color, 1, cv::LINE_AA);
    /*cv::drawMarker(image, point, imgutils::Red, cv::MARKER_SQUARE, 2);
    cv::drawMarker(image, offset_point + cv::Point(0, baseline), imgutils::Green, cv::MARKER_SQUARE, 2); //Enable for debugging*/
  }
}
