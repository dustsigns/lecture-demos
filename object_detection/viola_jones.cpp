//Illustration of Viola-Jones object detection
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>

#include "colors.hpp"
#include "window.hpp"

static bool InitClassifier(cv::CascadeClassifier &classifier)
{
  constexpr auto classifier_path = "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml";
  const bool successful = classifier.load(classifier_path);
  return successful;
}

static int OpenVideo(const char * const filename, cv::VideoCapture &capture)
{
  using namespace std::string_literals;
  const bool use_webcam = "-"s == filename; //Interpret - as the default webcam
  const bool opened = use_webcam ? capture.open(0) : capture.open(filename);
  if (use_webcam) //Minimize buffering for webcams to return up-to-date images
    capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
  return opened;
}

static void FindFaces(const cv::Mat &image, cv::CascadeClassifier &classifier, std::vector<cv::Rect> &faces)
{
  classifier.detectMultiScale(image, faces);
}

static void HighlightFaces(cv::Mat &image, std::vector<cv::Rect> faces)
{
  constexpr auto line_width = 2;
  const auto face_color = imgutils::Red;
  for (const auto &face : faces)
    cv::rectangle(image, face, face_color, line_width);
}

static cv::Mat GetGrayscaleImage(const cv::Mat &image)
{
  assert(image.type() == CV_8UC3); //Assume 8-bit RGB image
  cv::Mat grayscale_image;
  cv::cvtColor(image, grayscale_image, cv::COLOR_BGR2GRAY);
  return grayscale_image;
}

static void ShowFacesInImage(const cv::Mat &image, cv::CascadeClassifier &classifier, imgutils::Window &window)
{
  const cv::Mat grayscale_image = GetGrayscaleImage(image);
  std::vector<cv::Rect> faces;
  FindFaces(grayscale_image, classifier, faces);
  cv::Mat image_with_faces = image.clone();
  HighlightFaces(image_with_faces, faces);
  window.UpdateContent(image_with_faces);
}

static void ShowImages(cv::VideoCapture &capture, cv::CascadeClassifier &classifier, const int wait_time)
{
  constexpr auto window_name = "Frame with objects to detect";
  imgutils::Window window(window_name);
  
  cv::Mat frame;
  while (capture.read(frame) && !frame.empty())
  {
    ShowFacesInImage(frame, classifier, window);
    if (window.ShowInteractive(nullptr, wait_time, false) == 'q') //Do not hide window after each image; interpret Q key press as exit
      break;
  }
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2 && argc != 3)
  {
    std::cout << "Illustrates object detection on video frames with the object detector by Viola and Jones." << std::endl;
    std::cout << "Usage: " << argv[0] << " <input video> [<wait time between frames>]" << std::endl;
    return 1;
  }
  cv::CascadeClassifier classifier;
  if (!InitClassifier(classifier))
  {
    std::cerr << "Could not initialize cascade classifier" << std::endl;
    return 2;
  }
  const auto video_filename = argv[1];
  int wait_time = 0;
  if (argc == 3)
  {
    const auto wait_time_text = argv[2];
    wait_time = std::stoi(wait_time_text);
  }
  cv::VideoCapture capture;
  if (!OpenVideo(video_filename, capture))
  {
    std::cerr << "Could not open video '" << video_filename << "'" << std::endl;
    return 3;
  }
  ShowImages(capture, classifier, wait_time);
  return 0;
}
