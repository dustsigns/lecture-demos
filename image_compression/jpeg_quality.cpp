//Illustration of JPEG quality levels
// Andreas Unterweger, 2016-2019
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"
#include "imgmath.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

struct JPEG_data
{
  const Mat image;
  int quality;
  
  const string image_window_name;
  const string difference_window_name;
  
  JPEG_data(const Mat &image, const string &image_window_name, const string &difference_window_name)
   : image(image),
     quality(0),
     image_window_name(image_window_name), difference_window_name(difference_window_name) { }
};

static Mat CompressImage(const Mat &image, const unsigned char quality, unsigned int &compressed_size)
{
  assert(quality <= 100);
  vector<uchar> compressed_bits;
  assert(imencode(".jpg", image, compressed_bits, vector<int>({ImwriteFlags::IMWRITE_JPEG_QUALITY, quality, ImwriteFlags::IMWRITE_JPEG_OPTIMIZE, 1})));
  compressed_size = compressed_bits.size();
  const Mat compressed_image = imdecode(compressed_bits, ImreadModes::IMREAD_COLOR);
  assert(!compressed_image.empty());
  return compressed_image;
}

static Mat GetYChannelFromRGBImage(const Mat &image)
{
  Mat image_ycrcb;
  cvtColor(image, image_ycrcb, COLOR_BGR2YCrCb); //Convert to YCbCr color space
  Mat ycrcb_image_parts[3]; //TODO: Avoid the splitting overhead (is there a direct conversion?)
  split(image_ycrcb, ycrcb_image_parts);
  return ycrcb_image_parts[0]; //Extract Y channel (ignore Cr and Cb)
}

static void ShowDifferenceImage(const string &window_name, const Mat &image, const Mat &compressed_image)
{
  const Mat image_y = GetYChannelFromRGBImage(image);
  const Mat compressed_image_y = GetYChannelFromRGBImage(compressed_image);
  const Mat difference_y = SubtractImages(compressed_image_y, image_y);
  const double YPSNR = PSNR(MSE(difference_y));
  const string status_text = "Y-PSNR: " + FormatLevel(YPSNR);
  displayStatusBar(window_name, status_text);
  displayOverlay(window_name, status_text, 1000);
  imshow(window_name, ConvertDifferenceImage(difference_y));
}

static void ShowImages(const Mat &image)
{
  constexpr auto image_window_name = "Uncompressed vs. JPEG compressed";
  namedWindow(image_window_name);
  moveWindow(image_window_name, 0, 0);
  constexpr auto difference_window_name = "Difference";
  namedWindow(difference_window_name);
  moveWindow(difference_window_name, image.cols + 3, image.rows + 125); //Move difference window right below the right part of the comparison window (horizontal: image size plus 3 border pixels plus additional distance, vertial: image size plus additional distance for track bars etc.) //TODO: Get windows positions and calculate proper offsets
  static JPEG_data data(image, image_window_name, difference_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  constexpr auto trackbar_name = "Quality";
  createTrackbar(trackbar_name, image_window_name, &data.quality, 100,
    [](const int, void * const user_data)
      {
        auto &data = *((const JPEG_data * const)user_data);
        const Mat &image = data.image;
        const auto uncompressed_size = image.total() * image.elemSize();
        unsigned int compressed_size;
        Mat compressed_image = CompressImage(image, data.quality, compressed_size);
        const string status_text = FormatByte(uncompressed_size) + " vs. " + FormatByte(compressed_size);
        displayOverlay(data.image_window_name, status_text, 1000);
        displayStatusBar(data.image_window_name, status_text);
        const Mat combined_image = CombineImages({image, compressed_image}, Horizontal);
        imshow(data.image_window_name, combined_image);
        ShowDifferenceImage(data.difference_window_name, image, compressed_image);
      }, (void*)&data);
  setTrackbarPos(trackbar_name, image_window_name, 50); //Implies imshow with 50%
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates JPEG compression at different quality levels." << endl;
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
