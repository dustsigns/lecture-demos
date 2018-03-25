//Illustration of mean RGB feature vectors
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/viz.hpp>

using namespace std;

using namespace cv;
using namespace viz;

static void VisualizeCoordinateSystem(Viz3d &visualization)
{
  WCoordinateSystem coordinate_system;
  visualization.showWidget("Coordinate system", coordinate_system);
}

static Scalar GetMean(const Mat &image)
{
  assert(image.type() == CV_8UC3);
  const auto mean_value = mean(image);
  return mean_value / 256; //Normalize mean //TODO: Option to switch between this normalization and the one below
  /*const auto mean_sum = norm(mean_value, NORM_L1); //Alternative: normalize mean by sum
  return mean_value / mean_sum; //Normalize mean*/
}

static Mat FixImage(const Mat &image)
{
  Mat fixed_image;
  copyMakeBorder(image, fixed_image, 0, image.rows & 3, 0, image.cols & 3, BORDER_REFLECT_101, Scalar()); //Make sure each image dimension is a multiple of four
  return fixed_image;
}

static void VisualizeImage(const Mat &image, const string &name, Viz3d &visualization)
{
  constexpr auto thumbnail_size = 0.2;
  
  const auto feature = GetMean(image);
  const auto mean_b = feature[0];
  const auto mean_g = feature[1];
  const auto mean_r = feature[2];
  const Point3d position(mean_r, mean_g, mean_b);
  const Color vector_color(mean_b * 256, mean_g * 256, mean_r * 256);
  WArrow feature_vector(Point3d(), position, thumbnail_size / 100, vector_color);
  visualization.showWidget("feature" + name, feature_vector);
  
  const Mat fixed_image = FixImage(image); //Work around visualization constraints by slightly padding the image
  WImage3D thumbnail(fixed_image, Size2d(thumbnail_size, thumbnail_size), position, position, Vec3d(0, 0, 1));
  visualization.showWidget(name, thumbnail);
}

int main(const int argc, const char * const argv[])
{
  if (argc < 2)
  {
    cout << "Illustrates images and their feature vectors when using mean RGB features." << endl;
    cout << "Usage: " << argv[0] << " <input image 1> [<input image 2> ... [<input image n>]]" << endl;
    return 1;
  }
  Viz3d visualization("Feature vectors");
  VisualizeCoordinateSystem(visualization);
  vector<const char*> image_filenames(argv + 1, argv + argc); //Start from first actual parameter (ignore program name)
  for (const auto &image_filename : image_filenames)
  {
    const Mat image = imread(image_filename);
    if (image.empty())
    {
      cerr << "Could not read input image '" << image_filename << "'" << endl;
      return 3;
    }
    VisualizeImage(image, image_filename, visualization);
  }
  visualization.spin();
  return 0;
}
