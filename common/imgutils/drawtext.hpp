//Helper functions for drawing aligned text (header)
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <opencv2/core.hpp>

#include "colors.hpp"

namespace imgutils
{
  using namespace std;
  
  using namespace cv;
  
  static_assert(sizeof(unsigned char) == 1, "unsigned char must be 8 bits in size");
  
  //Way to align text
  enum TextAlignment : unsigned char
  {
    //No alignment
    None = 0x00,
    
    //Horizontal alignments
    Left = 0x01,
    Center = 0x02,
    Right = 0x04,
    
    //Vertical alignments
    Top = 0x10,
    Middle = 0x20,
    Bottom = 0x40,
    
    //Combined alignments
    TopLeft = Top | Left,
    TopCenter = Top | Center,
    TopRight = Top | Right,
    MiddleLeft = Middle | Left,
    MiddleCenter = Middle | Center,
    MiddleRight = Middle | Right,
    BottomLeft = Bottom | Left,
    BottomCenter = Bottom | Center,
    BottomRight = Bottom | Right,
  };
  
  //Draws the specified text with the defined alignment and parameters around the given point
  void DrawText(Mat_<Vec3b> &image, const string &text, const Point &point, const TextAlignment alignment, const Vec3b &color, const int cv_font_face, const double cv_font_scale);
}

