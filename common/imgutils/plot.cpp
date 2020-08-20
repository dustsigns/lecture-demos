//Helper class for plotting points and lines
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <algorithm>
#include <limits>

#include <opencv2/imgproc.hpp>

#include "colors.hpp"
#include "format.hpp"

#include "plot.hpp"

namespace imgutils
{
  using namespace std;

  using namespace cv;
  
  using namespace comutils;

  PointSet::PointSet(const vector<Point2d> &points, const Vec3b &point_color, const bool interconnect_points, const bool draw_samples, const bool draw_sample_bars, const unsigned int line_width)
   : points(points), point_color(point_color), interconnect_points(interconnect_points), draw_samples(draw_samples), draw_sample_bars(draw_sample_bars),
     line_width(line_width) { }
   
  Tick::Tick(const double position, const string &text, const bool text_visible) : position(position), text(text), text_visible(text_visible) { }
  
  void Tick::GenerateTicks(vector<Tick> &ticks, const double first, const double last, const double interval, const size_t label_every, const size_t decimal_places, const double conversion_factor)
  {
    assert(interval != 0 && label_every > 0);
    ticks.clear();
    const auto size = fabs(last - first) / fabs(interval) + 1; //Round down and add one (e.g., range from 0 to 9 should have 10 ticks)
    for (size_t i = 0; i < size; i++)
    {
      const double value = first + i * interval;
      const string text = FormatValue(value * conversion_factor, decimal_places);
      ticks.push_back(Tick(value, text, i % label_every == 0)); //Mark distance is interval and label each mark which is a multiple of label interval
    }
  }
  
  Plot::Plot(const vector<PointSet> &point_sets, const bool autoscale)
   : point_sets(point_sets), autoscale(autoscale),
     x_axis_label("x"), y_axis_label("y"),
     plotting(false) { }
     
  void Plot::SetVisibleRange(const Point2d &bottom_left, const Point2d &top_right)
  {
    constexpr double additional_border_factor = 0.1;
    static_assert(additional_border_factor > 0 && additional_border_factor < 1, "The absolute border size cannot be negative");
    assert(bottom_left.x <= top_right.x && bottom_left.y <= top_right.y);
    min_point = bottom_left - additional_border_factor * (top_right - bottom_left);
    max_point = top_right + additional_border_factor * (top_right - bottom_left);
  }
  
  void Plot::SetAutoscale(const bool autoscale)
  {
    this->autoscale = autoscale;
  }
  
  void Plot::SetAxesLabels(const string &x_axis_label, const string &y_axis_label)
  {
    this->x_axis_label = x_axis_label;
    this->y_axis_label = y_axis_label;
  }
  
  void Plot::SetMinMaxPoints()
  {
    constexpr auto max_double = numeric_limits<double>::max();
    double min_x = max_double, min_y = max_double;
    double max_x = -max_double, max_y = -max_double;
    for (const auto &point_set : point_sets)
    {
      const auto min_max_x = minmax_element(point_set.points.begin(), point_set.points.end(), [](const Point2d &a, const Point2d &b) //Find smallest and largest X coordinates
                                                                                                {
                                                                                                  return a.x < b.x;
                                                                                                });
      min_x = min(min_x, min_max_x.first->x);
      max_x = max(max_x, min_max_x.second->x);
      const auto min_max_y = minmax_element(point_set.points.begin(), point_set.points.end(), [](const Point2d &a, const Point2d &b) //Find smallest and largest Y coordinates
                                                                                                {
                                                                                                  return a.y < b.y;
                                                                                                });
      min_y = min(min_y, min_max_y.first->y);
      max_y = max(max_y, min_max_y.second->y);
    }
    min_x -= sample_bar_width / 2; //Consider bar width at the borders (worst case) when drawing samples
    max_x += sample_bar_width / 2;
    SetVisibleRange(Point2d(min_x, min_y), Point2d(max_x, max_y)); //TODO: Make sure axes are always visible; otherwise, drawing may fail later
  }
  
  void Plot::SetPlottingContext(const unsigned int width, const unsigned int height)
  {
    assert(width > 0);
    assert(height > 0);
    this->width = width;
    this->height = height;
   
    if (autoscale)
      SetMinMaxPoints();
    constexpr auto inf = numeric_limits<double>::infinity();
    const double plot_width = max_point.x - min_point.x;
    const double plot_height = max_point.y - min_point.y;
    assert(plot_width > 0 && plot_height > 0 && plot_width < inf && plot_height < inf);
    const Size2d scaling_factor((width - 1) / plot_width, (height - 1) / plot_height);
    assert(scaling_factor.width != 0 && scaling_factor.height != 0 && scaling_factor.width < inf && scaling_factor.height < inf);
    this->scaling_factor = scaling_factor;
    
    plotting = true;
  }
  
  Point Plot::ConvertPoint(const Point2d &point) const
  {
    assert(plotting); //If autoscaling is enabled, the parameters have had to be set so that they are sane
    Point2d converted_point(point - min_point);
    converted_point.x *= scaling_factor.width;
    converted_point.y *= scaling_factor.height;
    const Point integer_point(converted_point.x, height - 1 - converted_point.y); //Y axis is inverted (image would be upside down unless corrected)
    assert(integer_point.x >= 0 && integer_point.y >= 0 && integer_point.x < (int)width && integer_point.y < (int)height);
    return integer_point;
  }
  
  void Plot::UnsetPlottingContext()
  {
    plotting = false;
  }
  
  void Plot::DrawArrow(Mat_<Vec3b> &image, const Point &from, const Point &to, const Vec3b &color) const
  {
    const auto difference = to - from;
    const auto arrow_length = sqrt(difference.ddot(difference)); //Length is square root of inner product
    arrowedLine(image, from, to, color, 1, LINE_4, 0, arrow_size / arrow_length); //Work around bug where tip pixels are not drawn when using anti-aliasing
    arrowedLine(image, from, to, color, 1, LINE_AA, 0, arrow_size / arrow_length); //TODO: Get rid of extra drawing with LINE_4
  }
  
  void Plot::DrawLabel(Mat_<Vec3b> &image, const string &text, const Point &point, const TextAlignment alignment, const Vec3b &color) const
  {
    constexpr auto font = FONT_HERSHEY_TRIPLEX;
    constexpr auto font_size = 0.5;
    DrawText(image, text, point, alignment, color, font, font_size);
  }
  
  void Plot::DrawAxes(Mat_<Vec3b> &image) const
  {
    static const auto axis_color = Black;
    const Point x_axis_start = ConvertPoint(Point2d(min_point.x, 0));
    const Point y_axis_start = ConvertPoint(Point2d(0, min_point.y));
    const Point x_axis_end = ConvertPoint(Point2d(max_point.x, 0));
    const Point y_axis_end = ConvertPoint(Point2d(0, max_point.y));
    DrawArrow(image, x_axis_start, x_axis_end, axis_color);
    DrawArrow(image, x_axis_end, x_axis_start, axis_color);
    DrawArrow(image, y_axis_start, y_axis_end, axis_color);
    DrawArrow(image, y_axis_end, y_axis_start, axis_color);
    
    const Point x_axis_label_point = x_axis_end + Point(-label_offset, axis_label_offset); //Below
    const Point y_axis_label_point = y_axis_end + Point(-label_offset, axis_label_offset); //Left
    DrawLabel(image, x_axis_label, x_axis_label_point, BottomRight, axis_color);
    DrawLabel(image, y_axis_label, y_axis_label_point, BottomRight, axis_color);
  }
  
  void Plot::DrawTick(Mat_<Vec3b> &image, const Tick &tick, const bool x_tick) const
  {
    const auto ticks_color = Black;
    const Point2d tick_offset(x_tick ? tick.position : 0, x_tick ? 0 : tick.position);
    const Point tick_origin = ConvertPoint(tick_offset); //Position on axis
    const Point tick_start(x_tick ? tick_origin.x : tick_origin.x - tick_length / 2, //Left point
                           x_tick ? tick_origin.y - tick_length / 2 : tick_origin.y); //Lower point
    const Point tick_end(x_tick ? tick_origin.x : tick_origin.x + tick_length / 2, //Right point
                         x_tick ? tick_origin.y + tick_length / 2 : tick_origin.y); //Upper point
    line(image, tick_start, tick_end, ticks_color); //Tick through axis
    if (tick.text_visible)
    {
      const auto label_position = x_tick ? (tick_end + Point(0, label_offset)) : (tick_start + Point(-label_offset, 0)); //Below (X) or left (Y)
      DrawLabel(image, tick.text, label_position, x_tick ? BottomCenter : MiddleRight, ticks_color);
    }
  }
  
  void Plot::DrawTicks(Mat_<Vec3b> &image) const
  {
    for (const auto &tick : x_axis_ticks)
      DrawTick(image, tick, true);
    for (const auto &tick : y_axis_ticks)
      DrawTick(image, tick, false);
  }
  
  void Plot::DrawPointSets(Mat_<Vec3b> &image) const
  {
    for (const auto &point_set : point_sets)
    {
      const auto &points = point_set.points;
      vector<Point> converted_points(points.size());
      transform(points.begin(), points.end(), converted_points.begin(), [this](const Point2d &point)
                                                                              {
                                                                                return this->ConvertPoint(point);
                                                                              });
      if (point_set.interconnect_points) //Draw line from points
        polylines(image, converted_points, false, point_set.point_color, 1, LINE_AA); //TODO: Shift coordinates by one bit each for half-pixel accuracy
      if (point_set.draw_samples) //Draw each sample
      {
        for (const auto &point : converted_points)
        {
          const Point origin = ConvertPoint(Point2d(0, 0));
          const Point value_axis_position(point.x, origin.y);
          if (point_set.line_width == 1 || point_set.draw_sample_bars)
            line(image, value_axis_position, point, point_set.point_color, point_set.line_width); //Axis to point
          else
          {
            const Point rect_start(value_axis_position.x, value_axis_position.y);
            const Point rect_end(point.x + point_set.line_width, point.y);
            rectangle(image, rect_start, rect_end, point_set.point_color, FILLED);
          }
          if (point_set.draw_sample_bars)
          {
            const Point bar_start(point.x - point_set.line_width / 2 - sample_bar_width / 2, point.y); //Left point of bar
            const Point bar_end(point.x + point_set.line_width / 2 + sample_bar_width / 2, point.y); //Right point of bar
            line(image, bar_start, bar_end, point_set.point_color, point_set.line_width); //Bar through point
          }
        }
      }
    }
  }
  
  void Plot::DrawTo(Mat_<Vec3b> &rgb_image, const unsigned int width, const unsigned height)
  {
    const auto background_color = White;
    SetPlottingContext(width, height);
    rgb_image = Mat_<Vec3b>(height, width, background_color);
    //TODO: Grid?
    DrawAxes(rgb_image);
    DrawTicks(rgb_image);
    DrawPointSets(rgb_image);
    UnsetPlottingContext();
  }
}
