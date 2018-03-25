//Illustration of YCbCr color mixing
// Andreas Unterweger, 2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;

using namespace cv;

typedef struct YCbCr_data
{
  int y_portion;
  int cb_portion;
  int cr_portion;
  
  const string window_name;
  
  YCbCr_data(const string &window_name)
   : y_portion(0), cb_portion(0), cr_portion(0),
     window_name(window_name) { }
} YCbCr_data;

static Mat GenerateColorImage(const int y, const int cr, const int cb)
{
  constexpr auto image_dimension = 300;
  
  assert(y >= 0 && cr >= 0 && cb >= 0);
  assert(y <= 255 && cr <= 255 && cb <= 255); //TODO: Validate actual range
  
  const Size image_size(image_dimension, image_dimension);
  const Mat image(image_size, CV_8UC3, Scalar(y, cr, cb));
  return image;
}

static void UpdateImage(const int, void * const user_data)
{
  auto &data = *((const YCbCr_data * const)user_data);
  const Mat ycrcb_image = GenerateColorImage(data.y_portion, data.cr_portion, data.cb_portion);
  Mat rgb_image;
  cvtColor(ycrcb_image, rgb_image, COLOR_YCrCb2BGR);
  imshow(data.window_name, rgb_image);
}

static void ShowImage()
{
  constexpr auto window_name = "YCbCr color mixer";
  namedWindow(window_name, WINDOW_GUI_NORMAL);
  moveWindow(window_name, 0, 0);
  static YCbCr_data data(window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  constexpr auto y_trackbar_name = "Y portion";
  createTrackbar(y_trackbar_name, window_name, &data.y_portion, 255, UpdateImage, (void*)&data);
  constexpr auto cb_trackbar_name = "Cb portion";
  createTrackbar(cb_trackbar_name, window_name, &data.cb_portion, 255, UpdateImage, (void*)&data);
  constexpr auto cr_trackbar_name = "Cr portion";
  createTrackbar(cr_trackbar_name, window_name, &data.cr_portion, 255, UpdateImage, (void*)&data);
  setTrackbarPos(y_trackbar_name, window_name, 255); //Implies imshow with white (Y=255, Cb=128, Cr=128)
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates how YCbCr portions can be mixed into different colors." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  ShowImage();
  waitKey(0);
  return 0;
}
