//Illustration of chrominance subsampling
// Andreas Unterweger, 2016-2020
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "combine.hpp"
#include "format.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

struct color_format
{
  const string name;
  const int conversion_id;
  const int back_conversion_id;
  
  color_format(const string &name, const int conversion_id, const int back_conversion_id) 
   : name(name), conversion_id(conversion_id), back_conversion_id(back_conversion_id) { }
};

static const color_format color_formats[] {color_format("4:4:4", COLOR_BGR2YUV, COLOR_YUV2BGR),
                                           color_format("4:2:0", COLOR_BGR2YUV_I420, COLOR_YUV2BGR_I420),
                                           /*color_format("4:2:2", COLOR_BGR2YUV_Y422, COLOR_YUV2BGR_Y422)*/ //TODO: Find a conversion ID for BGR to YCbCr 4:2:2 and change order
                                           color_format("4:0:0", COLOR_BGR2GRAY, COLOR_GRAY2BGR)};

const auto &default_color_format = color_formats[1]; //4:2:0 by default

struct subsampling_data
{
  const Mat image;
  
  const string window_name;
  
  subsampling_data(const Mat &image, const string &window_name) 
   : image(image), window_name(window_name) { }
};

static Mat ConvertImage(const Mat &image, const color_format format, unsigned int &converted_size)
{
  Mat converted_image;
  assert(!image.empty());
  cvtColor(image, converted_image, format.conversion_id); //Convert to subsampled colorspace
  converted_size = converted_image.total() * converted_image.elemSize();
  cvtColor(converted_image, converted_image, format.back_conversion_id); //Convert back
  return converted_image;
}

static void ShowImage(const Mat &original_image)
{
  constexpr auto window_name = "Chrominance subsampling";
  namedWindow(window_name);
  moveWindow(window_name, 0, 0);
  static subsampling_data data(original_image, window_name); //Make variable visible within lambda (cannot capture since it couldn't be converted to a function pointer then)
  for (const auto &format : color_formats)
  {
    createButton(format.name,
      [](const int state, void * const user_data)
        {
          if (!state) //Ignore radio button events where the button becomes unchecked
            return;
          auto &format = *((const color_format * const)user_data);
          auto &image = data.image;
          auto &window_name = data.window_name;
          const auto uncompressed_size = image.total() * image.elemSize();
          unsigned int converted_size;
          const Mat converted_image = ConvertImage(image, format, converted_size);
          const string status_text = "4:4:4 (" + FormatByte(uncompressed_size) + ") vs. " + format.name + " (" + FormatByte(converted_size) + ")";
          displayOverlay(window_name, status_text, 1000);
          displayStatusBar(window_name, status_text);
          const Mat combined_image = CombineImages({image, converted_image}, Horizontal);
          imshow(window_name, combined_image);
        }, (void*)&format, QT_RADIOBOX, &format == &default_color_format); //Make radio button corresponding to default color format checked
  }
  //The first radio button is checked by default, triggering an update event implying imshow
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the effect of chrominance subsampling." << endl;
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
  ShowImage(image);
  waitKey(0);
  return 0;
}
