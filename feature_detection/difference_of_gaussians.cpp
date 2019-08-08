//Illustration of DoG computation
// Andreas Unterweger, 2017-2019
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"
#include "imgmath.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

struct DoG_data
{
  const Mat image;
  int k_percent;
  int sigma_percent;
  
  const string image_window_name;
  const string difference_window_name;
  
  DoG_data(const Mat &image, const string &image_window_name, const string &difference_window_name)
   : image(image),
     k_percent(100), sigma_percent(200),
     image_window_name(image_window_name), difference_window_name(difference_window_name) { }
};

static void ShowDifferenceImage(const string &window_name, const Mat &first_image, const Mat &second_image)
{
  assert(first_image.type() == CV_8UC1);
  assert(first_image.type() == CV_8UC1);
  const Mat difference_image = SubtractImages(first_image, second_image);
  imshow(window_name, ConvertDifferenceImage(difference_image));
}

static void UpdateImages(const int, void * const user_data)
{
  auto &data = *((const DoG_data * const)user_data);
  const Mat &image = data.image;
  const double sigma = data.sigma_percent / 100.0;
  const double k = data.k_percent / 100.0;
  Mat blurred_image_sigma, blurred_image_k_times_sigma;
  GaussianBlur(image, blurred_image_sigma, Size(), sigma);
  GaussianBlur(image, blurred_image_k_times_sigma, Size(), k * sigma);
  const Mat combined_image = CombineImages({blurred_image_sigma, blurred_image_k_times_sigma}, Horizontal);
  imshow(data.image_window_name, combined_image);
  ShowDifferenceImage(data.difference_window_name, blurred_image_k_times_sigma, blurred_image_sigma);
}

static const char *AddControls(DoG_data &data)
{
  constexpr auto max_sigma = 5.0;
  constexpr auto max_k = 5;
  
  constexpr auto sigma_trackbar_name = "Sigma [%]";
  createTrackbar(sigma_trackbar_name, data.image_window_name, &data.sigma_percent, max_sigma * 100, UpdateImages, (void*)&data);
  setTrackbarMin(sigma_trackbar_name, data.image_window_name, 1);
  constexpr auto k_trackbar_name = "k [%]";
  createTrackbar(k_trackbar_name, data.image_window_name, &data.k_percent, max_k * 100, UpdateImages, (void*)&data);
  setTrackbarMin(k_trackbar_name, data.image_window_name, 1);
  return k_trackbar_name;
}

static void ShowImages(const Mat &image)
{
  constexpr auto image_window_name = "Blurred (sigma), blurred (k * sigma)";
  namedWindow(image_window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(image_window_name, 0, 0);
  constexpr auto difference_window_name = "Blurred(k * sigma) - blurred (sigma)";
  namedWindow(difference_window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(difference_window_name, 2 * image.cols + 3 + 3, 0); //Move difference window right beside the comparison window (2 images plus 3 border pixels plus additional distance)
  static DoG_data data(image, image_window_name, difference_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  const auto main_parameter_trackbar_name = AddControls(data);
  setTrackbarPos(main_parameter_trackbar_name, image_window_name, 150); //Implies imshow with k=1.5
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the effect of the differences in sigma for the DoG." << endl;
    cout << "Usage: " << argv[0] << " <input image>" << endl;
    return 1;
  }
  const auto image_filename = argv[1];
  const Mat image = imread(image_filename, IMREAD_GRAYSCALE);
  if (image.empty())
  {
    cerr << "Could not read input image '" << image_filename << "'" << endl;
    return 2;
  }
  ShowImages(image);
  waitKey(0);
  return 0;
}
