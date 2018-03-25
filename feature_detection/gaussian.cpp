//Illustration of Gaussian filtering
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "common.hpp"
#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace comutils;
using namespace imgutils;

typedef struct Gaussian_data
{
  const Mat image;
  int sigma_percent;
  
  const string window_name;
  
  Gaussian_data(const Mat &image, const string &window_name)
   : image(image),
     sigma_percent(100),
     window_name(window_name) { }
} Gaussian_data;

static const char *AddControls(Gaussian_data &data)
{
  constexpr auto max_sigma = 20;
  constexpr auto scaling_trackbar_name = "Sigma [%]";
  createTrackbar(scaling_trackbar_name, data.window_name, &data.sigma_percent, max_sigma * 100,
                 [](const int, void * const user_data)
                   {
                     auto &data = *((const Gaussian_data * const)user_data);
                     const Mat &image = data.image;
                     const double sigma = data.sigma_percent / 100.0;
                     Mat blurred_image;
                     GaussianBlur(image, blurred_image, Size(), sigma);
                     const Mat combined_image = CombineImages({image, blurred_image}, Horizontal);
                     imshow(data.window_name, combined_image);
                   }, (void*)&data);
  setTrackbarMin(scaling_trackbar_name, data.window_name, 1);
  return scaling_trackbar_name;
}

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "Original vs. blurred";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  static Gaussian_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  const auto main_parameter_trackbar_name = AddControls(data);
  setTrackbarPos(main_parameter_trackbar_name, window_name, 200); //Implies imshow with sigma=2
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the effect of sigma of a Gaussian filter." << endl;
    cout << "Usage: " << argv[0] << " <input image>" << endl;
    return 1;
  }
  const auto filename = argv[1];
  const Mat image = imread(filename);
  if (image.empty())
  {
    cerr << "Could not read input image '" << filename << "'" << endl;
    return 2;
  }
  ShowImages(image);
  waitKey(0);
  return 0;
}
