//Illustration of non-linear lense distortion
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>

#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

typedef struct distortion_data
{
  const Mat image;
  int k1, k2, p1, p2;
  bool negative_coefficients;
  
  const string window_name;
  
  distortion_data(const Mat &image, const string &window_name)
   : image(image),
     k1(0), k2(0), p1(0), p2(0), negative_coefficients(false),
     window_name(window_name) { }
} distortion_data;

static Mat GetStandardCameraMatrix(Size2i image_size)
{
  Mat_<float> camera_matrix = Mat::eye(3, 3, CV_32F); //Focal lengths are 1
  camera_matrix(0, 2) = image_size.width / 2.f; //Principal point is at the center of the image
  camera_matrix(1, 2) = image_size.height / 2.f;
  return camera_matrix;
}

static void ShowDistortedImages(const int, void * const user_data)
{
  auto &data = *((const distortion_data * const)user_data);
  const Mat &image = data.image;
  const array<double, 4> distortion_vector { data.k1 * 1e-7, data.k2 * 1e-10, data.p1 * 1e-5, data.p2 * 1e-5 }; //TODO: Display scaling factors
  const auto standard_camera_matrix = GetStandardCameraMatrix(image.size());
  Mat distorted_image;
  if (data.negative_coefficients) //TODO: Is there a (working!) distort function (unlike the one in the fisheye namespace) which actually inverts the process?
    undistort(image, distorted_image, standard_camera_matrix, -Mat(distortion_vector));
  else
    undistort(image, distorted_image, standard_camera_matrix, distortion_vector);
  const Mat combined_image = CombineImages({image, distorted_image}, Horizontal);
  imshow(data.window_name, combined_image);
}

static const char *AddControls(distortion_data &data)
{
  createTrackbar("k1", data.window_name, &data.k1, 100, ShowDistortedImages, (void*)&data);
  createTrackbar("k2", data.window_name, &data.k2, 100, ShowDistortedImages, (void*)&data);
  createTrackbar("p1", data.window_name, &data.p1, 100, ShowDistortedImages, (void*)&data);
  createTrackbar("p2", data.window_name, &data.p2, 100, ShowDistortedImages, (void*)&data);
  createButton("Negative distortion coefficients", [](const int state, void * const user_data)
                                                     {
                                                       auto &data = *((distortion_data * const)user_data);
                                                       data.negative_coefficients = state != 0;
                                                       ShowDistortedImages(state, user_data);
                                                     }, (void*)&data, QT_CHECKBOX);
  return "k1";
}

static void ShowImages(const Mat &image)
{
  constexpr auto window_name = "Undistorted vs. distorted";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  static distortion_data data(image, window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  const auto main_parameter_trackbar_name = AddControls(data);
  setTrackbarPos(main_parameter_trackbar_name, window_name, 50); //Implies imshow with k1 = 5e-6
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the effect of the distortion vector on a camera image." << endl;
    cout << "Usage: " << argv[0] << " <camera image>" << endl;
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
