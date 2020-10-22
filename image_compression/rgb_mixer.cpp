//Illustration of RGB color mixing
// Andreas Unterweger, 2018-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

using namespace std;

using namespace cv;

struct RGB_data
{
  int r_portion;
  int g_portion;
  int b_portion;
  
  const string window_name;
  
  RGB_data(const string &window_name)
   : r_portion(0), g_portion(0), b_portion(0),
     window_name(window_name) { }
};

static Mat GenerateColorImage(const int r, const int g, const int b)
{
  constexpr auto image_dimension = 300;
  
  assert(r >= 0 && g >= 0 && b >= 0);
  assert(r <= 255 && g <= 255 && b <= 255);
  
  const Size image_size(image_dimension, image_dimension);
  const Mat image(image_size, CV_8UC3, Scalar(b, g, r));
  return image;
}

static void UpdateImage(const int, void * const user_data)
{
  auto &data = *(static_cast<const RGB_data*>(user_data));
  const Mat image = GenerateColorImage(data.r_portion, data.g_portion, data.b_portion);
  imshow(data.window_name, image);
}

static void ShowImage()
{
  constexpr auto window_name = "RGB color mixer";
  namedWindow(window_name, WINDOW_GUI_NORMAL);
  moveWindow(window_name, 0, 0);
  static RGB_data data(window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  constexpr auto r_trackbar_name = "R portion";
  createTrackbar(r_trackbar_name, window_name, &data.r_portion, 255, UpdateImage, static_cast<void*>(&data));
  constexpr auto g_trackbar_name = "G portion";
  createTrackbar(g_trackbar_name, window_name, &data.g_portion, 255, UpdateImage, static_cast<void*>(&data));
  constexpr auto b_trackbar_name = "B portion";
  createTrackbar(b_trackbar_name, window_name, &data.b_portion, 255, UpdateImage, static_cast<void*>(&data));
  setTrackbarPos(r_trackbar_name, window_name, 255); //Implies imshow with red (R=255, G=0, B=0)
}

int main(const int argc, const char * const argv[])
{
  if (argc != 1)
  {
    cout << "Illustrates how RGB portions can be mixed into different colors." << endl;
    cout << "Usage: " << argv[0] << endl;
    return 1;
  }
  ShowImage();
  waitKey(0);
  return 0;
}
