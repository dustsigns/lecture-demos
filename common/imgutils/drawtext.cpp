//Helper functions for drawing aligned text
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <opencv2/imgproc.hpp>

#include "drawtext.hpp"

namespace imgutils
{
  using namespace cv;
  
  void DrawText(Mat_<Vec3b> &image, const string &text, const Point &point, const TextAlignment alignment, const Vec3b &color, const int cv_font_face, const double cv_font_scale)
  {
    const auto text_size = getTextSize(text, cv_font_face, cv_font_scale, 1, NULL); //TODO: Consider baseline
    auto offset_point = point; //Default is top-left (no offset)
    
    if ((alignment & Center) == Center)
      offset_point.x -= text_size.width / 2;
    if ((alignment & Right) == Right)
      offset_point.x -= text_size.width;
    if ((alignment & Middle) == Middle)
      offset_point.y += text_size.height / 2;
    if ((alignment & Bottom) == Bottom)
      offset_point.y += text_size.height;
    putText(image, text, offset_point, cv_font_face, cv_font_scale, color, 1, LINE_AA);
    /*drawMarker(image, point, Red, MARKER_SQUARE, 2);
    drawMarker(image, offset_point, Green, MARKER_SQUARE, 2);*/ //Enable for debugging
  }
}
