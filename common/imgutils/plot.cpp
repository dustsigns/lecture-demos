//Helper class for plotting points and lines
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <algorithm>
#include <limits>
#include <numeric>

#include <opencv2/imgproc.hpp>

#include "colors.hpp"
#include "format.hpp"

#include "plot.hpp"

namespace imgutils
{
  PointSet::PointSet(const std::vector<cv::Point2d> &points, const cv::Vec3b &point_color, const bool interconnect_points, const bool draw_sample_bars, const unsigned int line_width)
   : points(points), point_color(point_color), interconnect_points(interconnect_points), draw_sample_bars(draw_sample_bars),
     line_width(line_width) { }
   
  Tick::Tick(const double position, const std::string &text, const bool text_visible) : position(position), text(text), text_visible(text_visible) { }
  
  void Tick::GenerateTicks(std::vector<Tick> &ticks, const double first, const double last, const double interval, const size_t label_every, const size_t decimal_places, const double conversion_factor)
  {
    assert(interval != 0 && label_every > 0);
    ticks.clear();
    const auto size = fabs(last - first) / fabs(interval) + 1; //Round down and add one (e.g., range from 0 to 9 should have 10 ticks)
    for (size_t i = 0; i < size; i++)
    {
      const double value = first + i * interval;
      const std::string text = comutils::FormatValue(value * conversion_factor, decimal_places);
      ticks.push_back(Tick(value, text, i % label_every == 0)); //Mark distance is interval and label each mark which is a multiple of label interval
    }
  }
  
  Plot::Plot(const std::vector<PointSet> &point_sets, const bool autoscale)
   : point_sets(point_sets), autoscale(autoscale), small_borders(false),
     x_axis_label("x"), y_axis_label("y"),
     plotting(false) { }

  void Plot::SetVisibleRange(const cv::Point2d &bottom_left, const cv::Point2d &top_right)
  {
    SetCoordinateRange(bottom_left, top_right); 
  }
  
  void Plot::SetCoordinateRange(const cv::Point2d &bottom_left, const cv::Point2d &top_right, const bool consider_border)
  {
    assert(bottom_left.x <= top_right.x && bottom_left.y <= top_right.y);
    if (consider_border)
    {
      const auto additional_border_factor = small_borders ? small_border_factor * default_border_factor : default_border_factor; //Reduce border to one quarter for small borders
      min_point = bottom_left - additional_border_factor * (top_right - bottom_left);
      max_point = top_right + additional_border_factor * (top_right - bottom_left);      
    }
    else
    {
      min_point = bottom_left;
      max_point = top_right;
    }
  }
  
  void Plot::GetVisibleRange(cv::Point2d &bottom_left, cv::Point2d &top_right) const
  {
    constexpr bool consider_border = true; //SetVisibleRange implicity assumes this as well, so set it for consistency
    if (consider_border)
    {
      const auto additional_border_factor = small_borders ? small_border_factor * default_border_factor : default_border_factor; //Reduce border to one quarter for small borders
      bottom_left = (min_point + additional_border_factor * top_right) / (1 + additional_border_factor);
      top_right = (max_point + additional_border_factor * bottom_left) / (1 + additional_border_factor);
    }
    else
    {
      bottom_left = min_point;
      top_right = max_point;
    }
  }
  
  void Plot::SetAutoscale(const bool autoscale)
  {
    this->autoscale = autoscale;
  }
  
  void Plot::SetSmallBorders(const bool small_borders)
  {
    this->small_borders = small_borders;
  }
  
  void Plot::SetAxesLabels(const std::string &x_axis_label, const std::string &y_axis_label)
  {
    this->x_axis_label = x_axis_label;
    this->y_axis_label = y_axis_label;
  }
  
  void Plot::GetPointSetsLimits(std::vector<coordinate_limits> &limits) const
  {
    for (const auto &point_set : point_sets)
    {
      if (point_set.points.empty())
        continue;
      const auto min_max_x = std::minmax_element(point_set.points.begin(), point_set.points.end(), [](const cv::Point2d &a, const cv::Point2d &b) //Find smallest and largest X coordinates
                                                                                                     {
                                                                                                       return a.x < b.x;
                                                                                                     });
      const auto min_x = min_max_x.first->x;
      const auto max_x = min_max_x.second->x;
      const auto min_max_y = std::minmax_element(point_set.points.begin(), point_set.points.end(), [](const cv::Point2d &a, const cv::Point2d &b) //Find smallest and largest Y coordinates
                                                                                                     {
                                                                                                       return a.y < b.y;
                                                                                                     });
      const auto min_y = min_max_y.first->y;
      const auto max_y = min_max_y.second->y;
      const int min_x_correction_px = point_set.draw_sample_bars ? (sample_bar_width / 2 + (point_set.line_width - 1)) : (point_set.interconnect_points ? (point_set.line_width - 1) : 0); //Bar width in both directions (half size each) for samples, considering thick line width; thick line width for interconnected points; nothing for rectangles
      const int max_x_correction_px = point_set.draw_sample_bars ? (sample_bar_width / 2 + (point_set.line_width - 1)) : (point_set.interconnect_points ? (point_set.line_width - 1) : point_set.line_width); //Bar width in both directions (half size each) for samples, considering thick line width; thick line width for interconnected points; rectangle width for rectangles
      const int min_y_correction_px = point_set.draw_sample_bars || point_set.interconnect_points ? point_set.line_width - 1 : 0; //Consider thick line width only for samples and interconnected points
      const int max_y_correction_px = point_set.draw_sample_bars || point_set.interconnect_points ? point_set.line_width - 1 : 0; //Consider thick line width only for samples and interconnected points
      const coordinate_limits limit(min_x, min_y, max_x, max_y, min_x_correction_px, min_y_correction_px, max_x_correction_px, max_y_correction_px);
      limits.push_back(limit);
    }
  }
  
  void Plot::GetAxesLimits(std::vector<coordinate_limits> &limits) const
  {
    const auto diagonal_arrow_size = static_cast<int>(ceil(sqrt(2) * arrow_size / 2));
    int baseline;
    const auto x_axis_label_size = cv::getTextSize(x_axis_label, label_font, label_font_size, 1, &baseline);
    const auto x_axis_label_height = axis_label_offset + x_axis_label_size.height + baseline; //TODO: Consider width
    const coordinate_limits x_axis_limits(largest_coordinate, 0, smallest_coordinate, 0, 0, std::max(diagonal_arrow_size, x_axis_label_height), 0, diagonal_arrow_size); //X axis is at y_min = y_max = 0 with y-facing arrows in both directions (the one with the smallest y coordinate is where the axis label is)
    limits.push_back(x_axis_limits);
    const auto y_axis_label_size = cv::getTextSize(y_axis_label, label_font, label_font_size, 1, nullptr);
    const auto y_axis_label_width = label_offset + y_axis_label_size.width; //TODO: Consider height
    const coordinate_limits y_axis_limits(0, largest_coordinate, 0, smallest_coordinate, std::max(diagonal_arrow_size, y_axis_label_width), 0, diagonal_arrow_size, 0); //X axis is at x_min = x_max = 0 with x-facing arrows in both directions (the one with the smallest x coordinate is where the axis label is)
    limits.push_back(y_axis_limits);
    
    if (!x_axis_ticks.empty())
    {
      const auto min_max_x = std::minmax_element(x_axis_ticks.begin(), x_axis_ticks.end(), [](const Tick &a, const Tick &b) //Find smallest and largest X coordinates
                                                                                             {
                                                                                               return a.position < b.position;
                                                                                             });
      const auto &min_tick = *min_max_x.first;
      const auto &max_tick = *min_max_x.second;
      const auto min_x = min_tick.position;
      const auto max_x = max_tick.position;
      if (min_tick.text_visible)
      {
        const auto min_tick_text_size = cv::getTextSize(min_tick.text, label_font, label_font_size, 1, nullptr);
        const auto min_x_correction_px = min_tick_text_size.width / 2; //Consider width of first (centered) label
        const coordinate_limits min_tick_limits(min_x, largest_coordinate, min_x, smallest_coordinate, min_x_correction_px, 0, 0, 0);
        limits.push_back(min_tick_limits);
      }
      if (max_tick.text_visible)
      {
        const auto max_tick_text_size = cv::getTextSize(max_tick.text, label_font, label_font_size, 1, nullptr);
        const auto max_x_correction_px = max_tick_text_size.height / 2; //Consider width of last (centered) label
        const coordinate_limits max_tick_limits(max_x, largest_coordinate, max_x, smallest_coordinate, 0, 0, max_x_correction_px, 0);
        limits.push_back(max_tick_limits);
      }

      auto min_y_correction_px = tick_length / 2; //Tick length in both directions (half size each)
      const auto max_y_correction_px = tick_length / 2;
      const auto max_text_height = std::accumulate(y_axis_ticks.begin(), y_axis_ticks.end(), 0,
                                                  [](int max_text_height, const Tick &a)
                                                    {
                                                      if (!a.text_visible) //Ignore tick if its text is invisible
                                                        return max_text_height;
                                                      int baseline;
                                                      const auto a_text_size = cv::getTextSize(a.text, label_font, label_font_size, 1, &baseline);
                                                      return std::max(max_text_height, a_text_size.height + baseline);
                                                    }); 
      min_y_correction_px += (max_text_height == 0 ? 0 : label_offset + max_text_height); //Consider height of highest tick label (plus offset)
      const coordinate_limits x_ticks_limits(min_x, 0, max_x, 0, 0, min_y_correction_px, 0, max_y_correction_px);
      limits.push_back(x_ticks_limits);
    }
    
    if (!y_axis_ticks.empty())
    {    
      const auto min_max_y = std::minmax_element(y_axis_ticks.begin(), y_axis_ticks.end(), [](const Tick &a, const Tick &b) //Find smallest and largest Y coordinates
                                                                                             {
                                                                                               return a.position < b.position;
                                                                                             });
      const auto &min_tick = *min_max_y.first;
      const auto &max_tick = *min_max_y.second;
      const auto min_y = min_tick.position;
      const auto max_y = max_tick.position;
      if (min_tick.text_visible)
      {
        int baseline;
        const auto min_tick_text_size = cv::getTextSize(min_tick.text, label_font, label_font_size, 1, &baseline);
        const auto min_y_correction_px = min_tick_text_size.height / 2 + baseline; //Consider height of first (centered) label
        const coordinate_limits min_tick_limits(largest_coordinate, min_y, smallest_coordinate, min_y, 0, min_y_correction_px, 0, 0);
        limits.push_back(min_tick_limits);
      }
      if (max_tick.text_visible)
      {
        int baseline;
        const auto max_tick_text_size = cv::getTextSize(max_tick.text, label_font, label_font_size, 1, &baseline);
        const auto max_y_correction_px = max_tick_text_size.height / 2 + baseline; //Consider height of last (centered) label
        const coordinate_limits max_tick_limits(largest_coordinate, max_y, smallest_coordinate, max_y, 0, 0, 0, max_y_correction_px);
        limits.push_back(max_tick_limits);
      }
      
      auto min_x_correction_px = tick_length / 2; //Tick length in both directions (half size each)
      const auto max_x_correction_px = tick_length / 2;
      const auto max_text_width = std::accumulate(y_axis_ticks.begin(), y_axis_ticks.end(), 0,
                                                 [](int max_text_width, const Tick &a)
                                                   {
                                                     if (!a.text_visible) //Ignore tick if its text is invisible
                                                       return max_text_width;
                                                     const auto a_text_size = cv::getTextSize(a.text, label_font, label_font_size, 1, nullptr);
                                                     return std::max(max_text_width, a_text_size.width);
                                                   });
      min_x_correction_px += (max_text_width == 0 ? 0 : label_offset + max_text_width); //Consider width of longest tick label (plus offset)
      const coordinate_limits y_ticks_limits(0, min_y, 0, max_y, min_x_correction_px, 0, max_x_correction_px, 0);
      limits.push_back(y_ticks_limits);
    }
  }
  
  std::vector<Plot::coordinate_limits> Plot::GetLimits() const
  {
    std::vector<coordinate_limits> limits;
    GetPointSetsLimits(limits);
    GetAxesLimits(limits);
    return limits;
  }
  
  bool Plot::VerifyLimits(const std::vector<coordinate_limits> &limits) const
  {
    unsigned int below_min, above_max;
    for (const auto &limit : limits)
    {
      if (limit.min_x != largest_coordinate)
      {
        const auto min_border_x = TryConvertXCoordinate(limit.min_x);
        const auto min_x = min_border_x - limit.min_x_correction_px;
        if (!CheckConvertedXCoordinate(min_x))
        {
          GetConvertedXCoordinateLimits(min_x, below_min, above_max);
          if (below_min != 0 || above_max != 0)
            return false;
        }
      }
      if (limit.max_x != smallest_coordinate)
      {
        const auto max_border_x = TryConvertXCoordinate(limit.max_x);
        const auto max_x = max_border_x + limit.max_x_correction_px;
        if (!CheckConvertedXCoordinate(max_x))
        {
          GetConvertedXCoordinateLimits(max_x, below_min, above_max);
          if (below_min != 0 || above_max != 0)
            return false;
        }
      }
      if (limit.min_y != largest_coordinate)
      {
        const auto min_border_y = TryConvertYCoordinate(limit.min_y);
        const auto min_y = min_border_y + limit.min_y_correction_px;
        if (!CheckConvertedYCoordinate(min_y))
        {
          GetConvertedYCoordinateLimits(min_y, below_min, above_max);
          if (below_min != 0 || above_max != 0)
            return false;
        }
      }
      if (limit.max_y != smallest_coordinate)
      {
        const auto max_border_y = TryConvertYCoordinate(limit.max_y);
        const auto max_y = max_border_y - limit.max_y_correction_px;
        if (!CheckConvertedYCoordinate(max_y))
        {
          GetConvertedYCoordinateLimits(max_y, below_min, above_max);
          if (below_min != 0 || above_max != 0)
            return false;
        }
      }
    }
    return true;
  }
  
  void Plot::SetAutomaticLimits()
  {
    const auto limits = GetLimits();
    coordinate_limits current_limit;
    for (const auto &limit : limits) //Determine limits without considering additional pixel limits at first
    {
      current_limit.min_x = std::min(current_limit.min_x, limit.min_x);
      current_limit.min_y = std::min(current_limit.min_y, limit.min_y);
      current_limit.max_x = std::max(current_limit.max_x, limit.max_x);
      current_limit.max_y = std::max(current_limit.max_y, limit.max_y);
    }
    SetCoordinateRange(cv::Point2d(current_limit.min_x, current_limit.min_y), cv::Point2d(current_limit.max_x, current_limit.max_y), false); //Set limits without border
    SetScalingFactor();
    
    unsigned int below_min, above_max;
    for (const auto &limit : limits) //Check how many actual pixels of overflow exist in each direction
    {
      if (limit.min_x != largest_coordinate)
      {
        const auto min_border_x = TryConvertXCoordinate(limit.min_x);
        const auto min_x = min_border_x - static_cast<int>(limit.min_x_correction_px);
        if (!CheckConvertedXCoordinate(min_x))
        {
          GetConvertedXCoordinateLimits(min_x, below_min, above_max);
          assert(above_max == 0);
          current_limit.min_x_correction_px = std::max(current_limit.min_x_correction_px, below_min);
        }
      }
      if (limit.max_x != smallest_coordinate)
      {
        const auto max_border_x = TryConvertXCoordinate(limit.max_x);
        const auto max_x = max_border_x + static_cast<int>(limit.max_x_correction_px);
        if (!CheckConvertedXCoordinate(max_x))
        {
          GetConvertedXCoordinateLimits(max_x, below_min, above_max);
          assert(below_min == 0);
          current_limit.max_x_correction_px = std::max(current_limit.max_x_correction_px, above_max);
        }
      }
      if (limit.min_y != largest_coordinate)
      {
        const auto min_border_y = TryConvertYCoordinate(limit.min_y);
        const auto min_y = min_border_y + static_cast<int>(limit.min_y_correction_px);
        if (!CheckConvertedYCoordinate(min_y))
        {
          GetConvertedYCoordinateLimits(min_y, below_min, above_max);
          assert(below_min == 0);
          current_limit.min_y_correction_px = std::max(current_limit.min_y_correction_px, above_max);
        }
      }
      if (limit.max_y != smallest_coordinate)
      {
        const auto max_border_y = TryConvertYCoordinate(limit.max_y);
        const auto max_y = max_border_y - static_cast<int>(limit.max_y_correction_px);
        if (!CheckConvertedYCoordinate(max_y))
        {
          GetConvertedYCoordinateLimits(max_y, below_min, above_max);
          assert(above_max == 0);
          current_limit.max_y_correction_px = std::max(current_limit.max_y_correction_px, below_min);
        }
      }
    }

    //Set new x and y limits based on additional pixel limits.
    //The basic idea is to consider that min_x has to be reduced by min_x_correction_px, but in the new coordinate system with the new scaling factor, i.e.,
    //min_x' = min_x - min_x_correction_px / scaling_factor'.
    //Analogously, the same is true for max_x. The new scaling factor, scaling_factor', will be determined by the total new width, i.e.,
    //scaling_factor.width = max_x' - min_x'.
    //Thus, min_x' and max_x' become
    //min_x' = min_x - min_x_correction_px * (max_x' - min_x') / (w - 1)
    //max_x' = max_x + max_x_correction_px * (max_x' - min_x') / (w - 1)
    //Solving the two equations together yields the formulae below. The procedure for the Y limits is the same in principle.
    const auto denominator_x = static_cast<int>(current_limit.min_x_correction_px) + static_cast<int>(current_limit.max_x_correction_px) - static_cast<int>(width) + 1;
    const auto nominator_min_x = current_limit.max_x * static_cast<int>(current_limit.min_x_correction_px) + current_limit.min_x * static_cast<int>(current_limit.max_x_correction_px) - current_limit.min_x * static_cast<int>(width) - current_limit.min_x;
    const auto nominator_max_x = current_limit.max_x * static_cast<int>(current_limit.min_x_correction_px) - current_limit.max_x * static_cast<int>(width) + current_limit.max_x + current_limit.min_x * static_cast<int>(current_limit.max_x_correction_px);
    const auto denominator_y = static_cast<int>(current_limit.min_y_correction_px) + static_cast<int>(current_limit.max_y_correction_px) - static_cast<int>(height) + 1;
    const auto nominator_min_y = current_limit.max_y * static_cast<int>(current_limit.min_y_correction_px) + current_limit.min_y * static_cast<int>(current_limit.max_y_correction_px) - current_limit.min_y * static_cast<int>(height) - current_limit.min_y;
    const auto nominator_max_y = current_limit.max_y * static_cast<int>(current_limit.min_y_correction_px) - current_limit.max_y * static_cast<int>(height) + current_limit.max_y + current_limit.min_y * static_cast<int>(current_limit.max_y_correction_px);
    current_limit.min_x = static_cast<double>(nominator_min_x) / static_cast<double>(denominator_x);
    current_limit.max_x = static_cast<double>(nominator_max_x) / static_cast<double>(denominator_x);
    current_limit.min_y = static_cast<double>(nominator_min_y) / static_cast<double>(denominator_y);
    current_limit.max_y = static_cast<double>(nominator_max_y) / static_cast<double>(denominator_y);
    
    SetVisibleRange(cv::Point2d(current_limit.min_x, current_limit.min_y), cv::Point2d(current_limit.max_x, current_limit.max_y));
    SetScalingFactor();
    assert(VerifyLimits(limits));
  }
  
  void Plot::SetScalingFactor()
  {
    constexpr auto inf = std::numeric_limits<double>::infinity();
    const double plot_width = max_point.x - min_point.x;
    const double plot_height = max_point.y - min_point.y;
    assert(plot_width > 0 && plot_height > 0 && plot_width < inf && plot_height < inf);
    const cv::Size2d scaling_factor((width - 1) / plot_width, (height - 1) / plot_height);
    assert(scaling_factor.width != 0 && scaling_factor.height != 0 && scaling_factor.width < inf && scaling_factor.height < inf);
    this->scaling_factor = scaling_factor;
  }
  
  void Plot::SetPlottingContext(const unsigned int width, const unsigned int height)
  {
    assert(width > 1);
    assert(height > 1);
    this->width = width;
    this->height = height;
    if (autoscale)
      SetAutomaticLimits();
    else
      SetScalingFactor();
    plotting = true;
  }
  
  void Plot::GetConvertedXCoordinateLimits(const int &x, unsigned int &px_below_min, unsigned int &px_above_max, const unsigned int additional_scaling_factor) const
  {
    px_below_min = x >= 0 ? 0 : static_cast<unsigned int>(-x);
    px_above_max = x < static_cast<int>(width * additional_scaling_factor) ? 0 : static_cast<unsigned int>(x - (width - 1) * additional_scaling_factor);
  }
  
  void Plot::GetConvertedYCoordinateLimits(const int &y, unsigned int &px_below_min, unsigned int &px_above_max, const unsigned int additional_scaling_factor) const
  {
    px_below_min = y >= 0 ? 0 : static_cast<unsigned int>(-y);
    px_above_max = y < static_cast<int>(height * additional_scaling_factor) ? 0 : static_cast<unsigned int>(y - (height - 1) * additional_scaling_factor);
  }
  
  bool Plot::CheckConvertedXCoordinate(const int &x, const unsigned int additional_scaling_factor) const
  {
    return x >= 0 && x < static_cast<int>(width * additional_scaling_factor);
  }
  
  bool Plot::CheckConvertedYCoordinate(const int &y, const unsigned int additional_scaling_factor) const
  {
    return y >= 0 && y < static_cast<int>(height * additional_scaling_factor);
  }

  int Plot::TryConvertXCoordinate(const double &x, const unsigned int additional_scaling_factor) const
  {
    const auto x_shifted = x - min_point.x;
    const auto x_scaled = x_shifted * scaling_factor.width;
    const int x_converted = round(x_scaled * additional_scaling_factor);
    return x_converted;
  }
  
  int Plot::TryConvertYCoordinate(const double &y, const unsigned int additional_scaling_factor) const
  {
    const auto y_shifted = y - min_point.y;
    const auto y_scaled = y_shifted * scaling_factor.height;
    const int y_converted = round((height - 1 - y_scaled) * additional_scaling_factor); //Y axis is inverted (image would be upside down unless corrected)
    return y_converted;
  }
  
  cv::Point Plot::ConvertPoint(const cv::Point2d &point, const unsigned int additional_scaling_factor) const
  {
    assert(plotting); //If autoscaling is enabled, the parameters have had to be set so that they are sane
    const auto x = TryConvertXCoordinate(point.x, additional_scaling_factor);
    const auto y = TryConvertYCoordinate(point.y, additional_scaling_factor);
    assert(CheckConvertedXCoordinate(x, additional_scaling_factor) && CheckConvertedYCoordinate(y, additional_scaling_factor));
    const cv::Point converted_point(x, y);
    return converted_point;
  }
  
  int Plot::GetVisibleXCoordinate(const double &x) const
  {
    assert(plotting); //If autoscaling is enabled, the parameters have had to be set so that they are sane
    return TryConvertXCoordinate(x);
  }
  
  int Plot::GetVisibleYCoordinate(const double &y) const
  {
    assert(plotting); //If autoscaling is enabled, the parameters have had to be set so that they are sane
    return TryConvertYCoordinate(y);
  }
  
  void Plot::UnsetPlottingContext()
  {
    plotting = false;
  }
  
  void Plot::DrawArrow(cv::Mat_<cv::Vec3b> &image, const cv::Point &from, const cv::Point &to, const cv::Vec3b &color) const
  {
    const auto difference = to - from;
    const auto arrow_length = sqrt(difference.ddot(difference)); //Length is square root of inner product
    cv::arrowedLine(image, from, to, color, 1, cv::LINE_AA, 0, arrow_size / arrow_length);
  }
  
  void Plot::DrawLabel(cv::Mat_<cv::Vec3b> &image, const std::string &text, const cv::Point &point, const TextAlignment alignment, const cv::Vec3b &color) const
  {
    DrawText(image, text, point, alignment, color, label_font, label_font_size);
  }
  
  void Plot::DrawAxes(cv::Mat_<cv::Vec3b> &image) const
  {
    static const auto axis_color = imgutils::Black;
    const cv::Point x_axis_start = ConvertPoint(cv::Point2d(min_point.x, 0));
    const cv::Point y_axis_start = ConvertPoint(cv::Point2d(0, min_point.y));
    const cv::Point x_axis_end = ConvertPoint(cv::Point2d(max_point.x, 0));
    const cv::Point y_axis_end = ConvertPoint(cv::Point2d(0, max_point.y));
    DrawArrow(image, x_axis_start, x_axis_end, axis_color);
    DrawArrow(image, x_axis_end, x_axis_start, axis_color);
    DrawArrow(image, y_axis_start, y_axis_end, axis_color);
    DrawArrow(image, y_axis_end, y_axis_start, axis_color);
    
    const cv::Point x_axis_label_point = x_axis_end + cv::Point(-label_offset, axis_label_offset); //Below
    const cv::Point y_axis_label_point = y_axis_end + cv::Point(-label_offset, axis_label_offset); //Left
    DrawLabel(image, x_axis_label, x_axis_label_point, BottomRight, axis_color);
    DrawLabel(image, y_axis_label, y_axis_label_point, BottomRight, axis_color);
  }
  
  void Plot::DrawTick(cv::Mat_<cv::Vec3b> &image, const Tick &tick, const bool x_tick) const
  {
    const auto ticks_color = imgutils::Black;
    const cv::Point2d tick_offset(x_tick ? tick.position : 0, x_tick ? 0 : tick.position);
    const cv::Point tick_origin = ConvertPoint(tick_offset); //Position on axis
    const cv::Point tick_start(x_tick ? tick_origin.x : tick_origin.x - tick_length / 2, //Left point
                               x_tick ? tick_origin.y - tick_length / 2 : tick_origin.y); //Lower point
    const cv::Point tick_end(x_tick ? tick_origin.x : tick_origin.x + tick_length / 2, //Right point
                             x_tick ? tick_origin.y + tick_length / 2 : tick_origin.y); //Upper point
    line(image, tick_start, tick_end, ticks_color); //Tick through axis
    if (tick.text_visible)
    {
      const auto label_position = x_tick ? (tick_end + cv::Point(0, label_offset)) : (tick_start + cv::Point(-label_offset, 0)); //Below (X) or left (Y)
      DrawLabel(image, tick.text, label_position, x_tick ? BottomCenter : MiddleRight, ticks_color); 
    }
  }
  
  void Plot::DrawTicks(cv::Mat_<cv::Vec3b> &image) const
  {
    for (const auto &tick : x_axis_ticks)
      DrawTick(image, tick, true);
    for (const auto &tick : y_axis_ticks)
      DrawTick(image, tick, false);
  }
  
  void Plot::DrawPointSets(cv::Mat_<cv::Vec3b> &image) const
  {
    for (const auto &point_set : point_sets)
    {
      const auto &points = point_set.points;
      std::vector<cv::Point> converted_points(points.size());
      if (point_set.interconnect_points) //Draw line from points
      {
        constexpr auto additional_accuracy_bits = 2u; //Quarter-pixel accuracy (2 bits)
        constexpr auto additional_accuracy_factor = 1u << additional_accuracy_bits;
        std::transform(points.begin(), points.end(), converted_points.begin(), [this](const cv::Point2d &point)
                                                                                     {
                                                                                       return this->ConvertPoint(point, additional_accuracy_factor);
                                                                                     });
        cv::polylines(image, converted_points, false, point_set.point_color, 1, cv::LINE_AA, additional_accuracy_bits); //Shift coordinates by multiple bits each for fractional pixel accuracy
      }
      else //Separate points (samples)
      {
        std::transform(points.begin(), points.end(), converted_points.begin(), [this](const cv::Point2d &point)
                                                                                     {
                                                                                       return this->ConvertPoint(point);
                                                                                     });
        for (const auto &point : converted_points)
        {
          const cv::Point origin = ConvertPoint(cv::Point2d(0, 0));
          const cv::Point value_axis_position(point.x, origin.y);
          if (point_set.line_width == 1 || point_set.draw_sample_bars)
            cv::line(image, value_axis_position, point, point_set.point_color, point_set.line_width); //Axis to point
          else //Filled rectangle
          {
            const cv::Point rect_start(value_axis_position.x, value_axis_position.y);
            const cv::Point rect_end(point.x + point_set.line_width, point.y);
            cv::rectangle(image, rect_start, rect_end, point_set.point_color, cv::FILLED);
          }
          if (point_set.draw_sample_bars)
          {
            const cv::Point bar_start(point.x - point_set.line_width / 2 - sample_bar_width / 2, point.y); //Left point of bar
            const cv::Point bar_end(point.x + point_set.line_width / 2 + sample_bar_width / 2, point.y); //Right point of bar
            cv::line(image, bar_start, bar_end, point_set.point_color, point_set.line_width); //Bar through point
          }
        }
      }
    }
  }
  
  void Plot::DrawTo(cv::Mat_<cv::Vec3b> &rgb_image, const unsigned int width, const unsigned height, std::function<PreRenderingCallback> pre_rendering_callback)
  {
    const auto background_color = White;
    SetPlottingContext(width, height);
    rgb_image = cv::Mat_<cv::Vec3b>(height, width, background_color);
    //TODO: Grid?
    if (pre_rendering_callback)
      pre_rendering_callback(*this);
    DrawAxes(rgb_image);
    DrawTicks(rgb_image);
    DrawPointSets(rgb_image);
    UnsetPlottingContext();
  }
}
