//Illustration of panoramic images
// Andreas Unterweger, 2017-2019
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>
#include <opencv2/highgui.hpp>

#include "colors.hpp"
#include "combine.hpp"

using namespace std;

using namespace cv;

using namespace imgutils;

static vector<Mat> LoadImages(vector<const char*> image_paths)
{
  vector<Mat> images(image_paths.size());
  transform(image_paths.begin(), image_paths.end(), images.begin(), 
    [](const char * const filename) -> Mat
      {
        const Mat image = imread(filename);
        if (image.empty())
          throw runtime_error("Could not read image '"s + filename + "'");
       return image;
      });
  return images;
}

static Mat StitchImages(const vector<Mat> &images)
{
  assert(images.size() >= 2);
  Mat stitched_image;
  auto stitcher = Stitcher::create();
  const auto status = stitcher->stitch(images, stitched_image);
  if (status != Stitcher::Status::OK)
    throw runtime_error("Stitching failed with error " + to_string(status));
  return stitched_image;
}

static void ShowImages(const vector<Mat> &images)
{
  constexpr auto window_name = "Images combined";
  namedWindow(window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  moveWindow(window_name, 0, 0);
  const Mat original_images = CombineImages(images.size(), images.data(), Horizontal);
  const Mat panoramic_image = StitchImages(images);
  const Mat combined_image = CombineImages({original_images, panoramic_image}, Vertical);
  imshow(window_name, combined_image);
}

int main(const int argc, const char * const argv[])
{
  if (argc < 3)
  {
    cerr << "Usage: " << argv[0] << " <first picture> <second picture> [<third picture> [ ... [<n-th picture>]]]" << endl;
    return 1;
  }
  vector<const char*> image_paths(argv + 1, argv + argc);
  try
  {
    const auto images = LoadImages(image_paths);
    ShowImages(images);
    waitKey(0);
  } catch (const runtime_error &e)
  {
    cerr << e.what() << endl;
  }
  return 0;
}
