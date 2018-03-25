//Illustration of RGB and YCbCr component decomposition
// Andreas Unterweger, 2016-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "RGB vs. YCbCr";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  Mat bgr_planes[3];
  split(image, bgr_planes);
  Mat ycrcb_image;
  cvtColor(image, ycrcb_image, COLOR_BGR2YCrCb); //TODO: Allow subsampling (how? cvtColor makes a 1-channel matrix with COLOR_BGR2YUV_*)
  Mat ycrcb_planes[3];
  split(ycrcb_image, ycrcb_planes);
  const Mat rgb_planes_combined = CombineImages({image, bgr_planes[2], bgr_planes[1], bgr_planes[0]}, Horizontal); //BGR as RGB
  const Mat ycbcr_planes_combined = CombineImages({image, ycrcb_planes[0], ycrcb_planes[2], ycrcb_planes[1]}, Horizontal); //YCrCb as YCbCr
  Mat combined_images = CombineImages({rgb_planes_combined, ycbcr_planes_combined}, Vertical);
  resize(combined_images, combined_images, Size(), 1 / sqrt(2), 1 / sqrt(2), INTER_LANCZOS4); //TODO: Don't resize, but find another way to fit the window(s) to the screen size, e.g., by allowing to hide the original image via a checkbox
  imshow(window_name, combined_images);
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Extracts the RGB and YCbCr channels of an image and displays them." << endl;
    cout << "Usage: " << argv[0] << " <input image>" << endl;
    return 1;
  }
  const auto image_filename = argv[1];
  const Mat image = imread(image_filename);
  if (image.empty())
  {
    cerr << "Could not read input image '" << image_filename << "'" << endl;
    return 2;
  }
  ShowImages(image);
  waitKey(0);
  return 0;
}
