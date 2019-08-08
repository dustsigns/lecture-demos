//Illustration of plane-to-plane warping with a homography
// Andreas Unterweger, 2017-2019
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "colors.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

struct homography_data
{
  const Mat source_image;
  const Mat target_image;
  
  vector<Point2f> target_corners;
  
  const string source_window_name;
  const string target_window_name;
  
  homography_data(const Mat &source_image, const Mat &target_image, const string &source_window_name, const string &target_window_name)
   : source_image(source_image), target_image(target_image), source_window_name(source_window_name), target_window_name(target_window_name) { }
};

static Mat DrawCorners(const Mat &original_image, const vector<Point2f> &corners)
{
  constexpr auto circle_radius = 3;
  const auto circle_color = Red;
  
  Mat new_image(original_image.clone()); //Copy original image
  for (const auto &corner : corners)
    circle(new_image, corner, circle_radius, circle_color, -1); //Filled circle
  return new_image;
}

static Mat WarpImage(const Mat &source_image, const Mat &target_image, const vector<Point2f> &target_corners)
{
  const auto max_x = source_image.cols - 1;
  const auto max_y = source_image.rows - 1;
  const Point2f top_left(0, 0);
  const Point2f top_right(max_x, 0);
  const Point2f bottom_right(max_x, max_y);
  const Point2f bottom_left(0, max_y);
  const vector<Point2f> source_corners({ top_left, top_right, bottom_right, bottom_left });
  
  const auto homography = getPerspectiveTransform(source_corners, target_corners);
  Mat warped_image;
  warpPerspective(source_image, warped_image, homography, target_image.size());
  
  const Mat image_mask = Mat(source_image.size(), CV_8UC1, Scalar(255)); //Create mask
  Mat image_mask_warped;
  warpPerspective(image_mask, image_mask_warped, homography, target_image.size()); //Warp mask so that unused pixels become zero
  Mat new_image(target_image.clone()); //Copy original image
  warped_image.copyTo(new_image, image_mask_warped); //Copy warped image into unused pixels
  return new_image;
}

static void ShowImages(const Mat &source_image, const Mat &target_image)
{
  constexpr auto source_window_name = "Source image";
  namedWindow(source_window_name);
  moveWindow(source_window_name, 0, 0);
  constexpr auto target_window_name = "Target image";
  namedWindow(target_window_name);
  moveWindow(target_window_name, source_image.cols + 3 + 3, 0); //Move target window right beside the source window (image size plus 3 border pixels plus additional distance)
  static homography_data data(source_image, target_image, source_window_name, target_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  setMouseCallback(target_window_name, [](const int event, const int x, const int y, int, void * const user_data)
                                         {
                                           if (event != EVENT_LBUTTONDOWN) //Only react on mouse down
                                             return;
                                           auto &data = *((homography_data * const)user_data);
                                           if (data.target_corners.size() < 4)
                                             data.target_corners.push_back(Point2f(x, y));
                                           const auto corner_image = DrawCorners(data.target_image, data.target_corners);
                                           imshow(data.target_window_name, corner_image);
                                           if (data.target_corners.size() == 4) //TODO: Make this more intuitive
                                           {
                                             const auto warped_image = WarpImage(data.source_image, data.target_image, data.target_corners);
                                             imshow(data.target_window_name, warped_image);
                                           }
                                         }, (void*)&data);
  auto clear_button_name = "Clear selection";
  createButton(clear_button_name, [](const int, void * const user_data)
                                    {
                                      auto &data = *((homography_data * const)user_data);
                                      data.target_corners.clear();
                                      imshow(data.target_window_name, data.target_image); //Redraw original image
                                    }, (void*)&data);
  imshow(source_window_name, source_image);
  imshow(target_window_name, target_image);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    cout << "Illustrates the warp in perspective from one plane to another." << endl;
    cout << "Usage: " << argv[0] << " <source image> <target image>" << endl;
    return 1;
  }
  const auto source_image_filename = argv[1];
  const Mat source_image = imread(source_image_filename);
  if (source_image.empty())
  {
    cerr << "Could not read source image '" << source_image_filename << "'" << endl;
    return 2;
  }
  const auto target_image_filename = argv[2];
  const Mat target_image = imread(target_image_filename);
  if (target_image.empty())
  {
    cerr << "Could not read target image '" << target_image_filename << "'" << endl;
    return 3;
  }
  ShowImages(source_image, target_image);
  waitKey(0);
  return 0;
}
