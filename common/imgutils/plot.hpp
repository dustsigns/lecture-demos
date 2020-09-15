//Helper class for plotting points and lines (header)
// Andreas Unterweger, 2017-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "drawtext.hpp"

namespace imgutils
{
  using namespace std;
  
  using namespace cv;
  
  //Represents a set of points to be plotted
  class PointSet
  {
    public:
      //Represents a list of points with X and Y coordinates
      vector<Point2d> points;

      //Determines the color of the points when visualized
      Vec3b point_color;
      //Determines whether the points are interconnected by lines when visualized
      bool interconnect_points;
      //Determines whether sample bars are drawn at the points when interconnect_points is disabled
      bool draw_sample_bars;
      
      //Determines the width of the line used when drawing lines for this point set, e.g., during interconnecting. When draw_sample_bars is disabled, a line_width greater than 1 will draw a rectangle
      unsigned int line_width;
    
      //Creates a new point set from the given points with the specified visualization parameters
      PointSet(const vector<Point2d> &points, const Vec3b &point_color = Vec3b(0, 0, 255), const bool interconnect_points = true, const bool draw_sample_bars = true, const unsigned int line_width = 1);
      //Creates a new point set from the given Y coordinates with the specified visualization parameters. The corresponding X coordinates are assumed to be 0..(points.size()-1) * x_scale.
      template<typename T>
      PointSet(const vector<T> &y_coordinates, const double x_scale = 1.0, const Vec3b &point_color = Vec3b(0, 0, 255), const bool interconnect_points = true, const bool draw_sample_bars = true, const unsigned int line_width = 1);
  };
  
  //Represents an axis mark with its position, associated text and the information whether the latter should be displayed
  struct Tick
  {
    //Determines the position of the axis mark
    double position;
    //Determines the text associated with the axis mark
    string text;
    //Determines whether or not the text is visible
    bool text_visible;
    
    //Creates a new axis mark with the specified position and text
    Tick(const double position, const string &text, const bool text_visible);
    
    //Fills a vector of ticks with ticks in the range between first and last, with interval space between them. Only every label_every label receives a label of the value*conversion factor, rounded to decimal_places decimal places.
    static void GenerateTicks(vector<Tick> &ticks, const double first, const double last, const double interval, const size_t label_every, const size_t decimal_places = 0, const double conversion_factor = 1.0);
  };
  
  //Plots points and lines
  class Plot
  {
    public:
      //Represents a list of point sets to be plotted
      vector<PointSet> point_sets;
      
      //Represents a list of ticks along the X axis
      vector<Tick> x_axis_ticks;
      //Represents a list of ticks along the Y axis
      vector<Tick> y_axis_ticks;
    
      //Creates a new plot from the given point sets
      Plot(const vector<PointSet> &point_sets, const bool autoscale = true);
      
      //Sets the bottom-left and top-right visible points when autoscaling is deactivated. By default, a border is added. Its size can be reduced by calling SetSmallBorders.
      void SetVisibleRange(const Point2d &bottom_left, const Point2d &top_right);
      
      //Enables or disables autoscaling. When autoscaling is disabled, SetVisibleRange has to be called before drawing.
      void SetAutoscale(const bool autoscale = true);
      
      //Enables or disables small borders when drawing.
      void SetSmallBorders(const bool small_borders = true);
      
      //Sets the X and Y axes' labels
      void SetAxesLabels(const string &x_axis_label, const string &y_axis_label);
      
      //Determines the default width of the output plot in pixels
      static constexpr auto default_width = 640u;
      //Determines the default height of the output plot in pixels
      static constexpr auto default_height = 480u;
      
      //Draws the plot onto an RGB image with 8 bits per channel. If autoscaling is deactivated, all plotted coordinates must be visible since no clipping is performed.
      void DrawTo(Mat_<Vec3b> &rgb_image, const unsigned int width = default_width, const unsigned int height = default_height);
    
    private:
      bool autoscale;
      bool small_borders;
      string x_axis_label;
      string y_axis_label;
      
      bool plotting;
      unsigned int width;
      unsigned int height;
      Point2d min_point;
      Point2d max_point;
      Size2d scaling_factor;
      
      static constexpr auto max_double = numeric_limits<double>::max();
      static constexpr auto smallest_coordinate = -max_double;
      static constexpr auto largest_coordinate = max_double;
      
      struct coordinate_limits
      {
        double min_x, min_y, max_x, max_y;
        unsigned int min_x_correction_px, min_y_correction_px, max_x_correction_px, max_y_correction_px;
        
        coordinate_limits(const double min_x = largest_coordinate, const double min_y = largest_coordinate, const double max_x = smallest_coordinate, const double max_y = smallest_coordinate, const unsigned int min_x_correction_px = 0, const unsigned int min_y_correction_px = 0, const unsigned int max_x_correction_px = 0, const unsigned int max_y_correction_px = 0) : min_x(min_x), min_y(min_y), max_x(max_x), max_y(max_y), min_x_correction_px(min_x_correction_px), min_y_correction_px(min_y_correction_px), max_x_correction_px(max_x_correction_px), max_y_correction_px(max_y_correction_px) { }
      };
      
      void GetPointSetsLimits(vector<coordinate_limits> &limits) const;
      void GetAxesLimits(vector<coordinate_limits> &limits) const;
      vector<coordinate_limits> GetLimits() const;
      bool VerifyLimits(const vector<coordinate_limits> &limits) const;
      void SetAutomaticLimits();
      void SetScalingFactor();
      void SetCoordinateRange(const Point2d &bottom_left, const Point2d &top_right, const bool consider_border = true);
      void GetConvertedXCoordinateLimits(const int &x, unsigned int &px_below_min, unsigned int &px_above_max, const unsigned int additional_scaling_factor = 1) const;
      void GetConvertedYCoordinateLimits(const int &y, unsigned int &px_below_min, unsigned int &px_above_max, const unsigned int additional_scaling_factor = 1) const;
      bool CheckConvertedXCoordinate(const int &x, const unsigned int additional_scaling_factor = 1) const;
      bool CheckConvertedYCoordinate(const int &y, const unsigned int additional_scaling_factor = 1) const;
      int TryConvertXCoordinate(const double &x, const unsigned int additional_scaling_factor = 1) const;
      int TryConvertYCoordinate(const double &y, const unsigned int additional_scaling_factor = 1) const;
      Point ConvertPoint(const Point2d &point, const unsigned int additional_scaling_factor = 1) const;
      double TryConvertXCoordinateBack(const int &x, const unsigned int additional_scaling_factor = 1) const;
      double TryConvertYCoordinateBack(const int &y, const unsigned int additional_scaling_factor = 1) const;
      void SetPlottingContext(const unsigned int width, const unsigned int height);
      void UnsetPlottingContext();
      void DrawArrow(Mat_<Vec3b> &image, const Point &from, const Point &to, const Vec3b &color) const;
      void DrawLabel(Mat_<Vec3b> &image, const string &text, const Point &point, const TextAlignment alignment, const Vec3b &color) const;
      void DrawAxes(Mat_<Vec3b> &image) const;
      void DrawTick(Mat_<Vec3b> &image, const Tick &tick, const bool x_tick) const;
      void DrawTicks(Mat_<Vec3b> &image) const;
      void DrawPointSets(Mat_<Vec3b> &image) const;
      
      static constexpr auto arrow_size = 10; //Arrow size in pixels
      static constexpr auto axis_label_offset = 10; //Offset in pixels
      static constexpr auto label_offset = 10 / 2; //Offset in pixels
      static constexpr auto tick_length = 10; //Tick length in pixels
      static constexpr auto sample_bar_width = 10; //Sample bar width in pixels
      static constexpr auto label_font = FONT_HERSHEY_TRIPLEX; //Font for axis and tick labels
      static constexpr auto label_font_size = 0.5; //Font size for axis and tick labels
  };
}

#include "plot.impl.hpp"

