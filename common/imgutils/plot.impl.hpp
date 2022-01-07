//Helper class for plotting points and lines (template implementation)
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <algorithm>

//#include "plot.hpp"

namespace imgutils
{
  template<typename T>
  PointSet::PointSet(const std::vector<T> &y_coordinates, const double x_scale, const cv::Vec3b &point_color, const bool interconnect_points, const bool draw_sample_bars, const unsigned int line_width)
   : point_color(point_color), interconnect_points(interconnect_points), draw_sample_bars(draw_sample_bars),
     line_width(line_width)
  {
    size_t i = 0;
    points.resize(y_coordinates.size());
    std::transform(y_coordinates.begin(), y_coordinates.end(), points.begin(), [&i, &x_scale](const T y_coordinate)
                                                                                             {
                                                                                               return cv::Point2d(x_scale * i++, y_coordinate);
                                                                                             });
  }
}
