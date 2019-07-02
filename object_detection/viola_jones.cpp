//Illustration of Viola-Jones object detection
// Andreas Unterweger, 2017-2019
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>

#include "colors.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

static bool InitClassifier(CascadeClassifier &classifier)
{
  constexpr auto classifier_path = "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml";
  const bool successful = classifier.load(classifier_path);
  return successful;
}

static void FindFaces(const Mat &image, /*const*/ CascadeClassifier &classifier, vector<Rect> &faces) //TODO: detectMultiScale should be const, but is not
{
  classifier.detectMultiScale(image, faces);
}

static void HighlightFaces(Mat &image, vector<Rect> faces)
{
  const auto face_color = Red;
  for (const auto &face : faces)
    rectangle(image, face, face_color);
}

static Mat GetColorImage(const Mat &image)
{
  Mat color_image;
  cvtColor(image, color_image, COLOR_GRAY2BGRA);
  return color_image;
}

static void ShowImage(const Mat &image, /*const*/ CascadeClassifier &classifier)
{
  constexpr auto window_name = "Image with objects to detect";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  vector<Rect> faces;
  FindFaces(image, classifier, faces);
  Mat image_with_faces = GetColorImage(image);
  HighlightFaces(image_with_faces, faces);
  imshow(window_name, image_with_faces);
}

int main(const int argc, const char * const argv[])
{
  if (argc < 2)
  {
    cout << "Illustrates object detection on multiple input frames with the object detector by Viola and Jones." << endl;
    cout << "Usage: " << argv[0] << " <input image 1> [<input image 2> ... [<input image n>]]" << endl;
    return 1;
  }
  CascadeClassifier classifier;
  if (!InitClassifier(classifier))
  {
    cerr << "Could not initialize cascade classifier" << endl;
    return 2;
  }
  vector<const char*> image_filenames(argv + 1, argv + argc); //Start from first actual parameter (ignore program name)
  for (const auto &image_filename : image_filenames)
  {
    Mat image = imread(image_filename, IMREAD_GRAYSCALE);
    if (image.empty())
    {
      cerr << "Could not read input image '" << image_filename << "'" << endl;
      return 3;
    }
    ShowImage(image, classifier);
    waitKey(0);
  }
  return 0;
}
