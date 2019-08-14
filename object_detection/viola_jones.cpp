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
  constexpr auto line_width = 2;
  const auto face_color = Red;
  for (const auto &face : faces)
    rectangle(image, face, face_color, line_width);
}

static Mat GetGrayscaleImage(const Mat &image)
{
  assert(image.type() == CV_8UC3); //Assume 8-bit RGB image
  Mat grayscale_image;
  cvtColor(image, grayscale_image, COLOR_BGR2GRAY);
  return grayscale_image;
}

static void ShowImage(const Mat &image, /*const*/ CascadeClassifier &classifier)
{
  constexpr auto window_name = "Frame with objects to detect";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  const Mat grayscale_image = GetGrayscaleImage(image);
  vector<Rect> faces;
  FindFaces(grayscale_image, classifier, faces);
  Mat image_with_faces = image.clone();
  HighlightFaces(image_with_faces, faces);
  imshow(window_name, image_with_faces);
}

static int OpenVideo(const char * const filename, VideoCapture &capture)
{
  const bool use_webcam = "-"s == filename; //Interpret - as the default webcam
  const bool opened = use_webcam ? capture.open(0) : capture.open(filename);
  if (use_webcam) //Minimize buffering for webcams to return up-to-date images
    capture.set(CAP_PROP_BUFFERSIZE, 1);
  return opened;
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2 && argc != 3)
  {
    cout << "Illustrates object detection on video frames with the object detector by Viola and Jones." << endl;
    cout << "Usage: " << argv[0] << " <input video> [<wait time between frames>]" << endl;
    return 1;
  }
  CascadeClassifier classifier;
  if (!InitClassifier(classifier))
  {
    cerr << "Could not initialize cascade classifier" << endl;
    return 2;
  }
  const auto video_filename = argv[1];
  int wait_time = 0;
  if (argc == 3)
  {
    const auto wait_time_text = argv[2];
    wait_time = stoi(wait_time_text);
  }
  VideoCapture capture;
  if (!OpenVideo(video_filename, capture))
  {
    cerr << "Could not open video '" << video_filename << "'" << endl;
    return 3;
  }
  Mat frame;
  while (capture.read(frame) && !frame.empty())
  {
    ShowImage(frame, classifier);
    if (waitKey(wait_time) == 'q') //Interpret Q key press as exit
      break;
  }
  return 0;
}
