//Image combination functions
// Andreas Unterweger, 2016-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <algorithm>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "common.hpp"
#include "combine.hpp"

namespace imgutils
{
  using namespace std;

  using namespace cv;

  Mat CombineImages(const size_t N, const Mat images[], const CombinationMode mode, const unsigned int border_size)
  {
    assert(N > 1);
    assert(mode == Horizontal || mode == Vertical);
    const bool horizontal = mode == Horizontal;
    assert(border_size != 0);
    bool grayscale = true; //Assume gray-scale images
    Size max_size(0, 0);
    for (size_t i = 0; i < N; i++)
    {
      auto &image = images[i];
      assert(image.type() == CV_8UC1 || image.type() == CV_8UC3);
      if (grayscale && image.type() == CV_8UC3) //If at least one image uses a format which supports colors, all others will be converted later
        grayscale = false;
      max_size.width = max(image.cols, max_size.width); //Use the largest image dimension
      max_size.height = max(image.rows, max_size.height);
    }
    const Mat black_bar(horizontal ? max_size.height : border_size, horizontal ? border_size : max_size.width, grayscale ? CV_8UC1 : CV_8UC3, grayscale ? Scalar(0) : Scalar(0, 0, 0));
    vector<Mat> image_parts(2 * N - 1);
    for (size_t i = 0; i < N; i++)
    {
      Mat current_image(images[i]);
      if (!grayscale && current_image.type() == CV_8UC1) //Image needs to be converted into a format which supports colors
      {
        Mat new_image;
        cvtColor(current_image, new_image, COLOR_GRAY2BGR);
        current_image = new_image;
      }
      if (current_image.size() != max_size) //Image needs to be resized
      {
        Mat new_image(max_size, grayscale ? CV_8UC1 : CV_8UC3, grayscale ? Scalar(0) : Scalar(0, 0, 0));
        current_image.copyTo(new_image(Rect(Point(), current_image.size()))); //Copy old image into top-left corner of new image
        current_image = new_image;
      }
      image_parts[2 * i] = current_image;
      if (i != N - 1) //No black bar after last image
        image_parts[2 * i + 1] = black_bar;
    }
    Mat combined_image;
    /*constexpr auto horizontal_combination_function = static_cast<void (*)(const Mat*, size_t, OutputArray)>(hconcat);
    constexpr auto vertical_combination_function = static_cast<void (*)(const Mat*, size_t, OutputArray)>(vconcat);
    (horizontal ? horizontal_combination_function : vertical_combination_function)(image_parts.data(), image_parts.size(), combined_image);*/
    if (horizontal)
      hconcat(image_parts.data(), image_parts.size(), combined_image);
    else
      vconcat(image_parts.data(), image_parts.size(), combined_image);
    return combined_image;
  }

  Mat SubtractImages(const Mat &image1, const Mat &image2)
  {
    assert(image1.type() == CV_8UC1 && image2.type() == CV_8UC1);
    Mat image1_16;
    image1.convertTo(image1_16, CV_16SC1); //Convert 8 bit unsigned to 16 bit signed to allow for negative values within the maximum possible range
    Mat image2_16;
    image2.convertTo(image2_16, CV_16SC1);
    return image1_16 - image2_16;
  }

  Mat ConvertDifferenceImage(const Mat &difference_image, DifferenceConversionMode mode)
  {
    assert(difference_image.type() == CV_16SC1);
    static_assert(sizeof(short) == 2, "short needs to be 16 bits in size");
    Mat difference(difference_image.size(), mode == Color ? CV_8UC3 : CV_8UC1);
    switch (mode)
    {
      case Offset:
         difference_image.convertTo(difference, CV_8UC1, 1, 128); //Add 128 to each pixel
         break;
      case Absolute:
        {
          auto &difference_image_16 = static_cast<const Mat_<short>>(difference_image);
          auto difference_gray = static_cast<Mat_<unsigned char>>(difference);
          transform(difference_image_16.begin(), difference_image_16.end(), difference_gray.begin(),
                    [](const short value)
                      {
                        const unsigned short absolute_value = abs(value);
                        return min(absolute_value, static_cast<unsigned short>(255));
                      });
        }
        break;
      case Color:
      default:
        {
          auto &difference_image_16 = static_cast<const Mat_<short>>(difference_image);
          auto difference_RGB = static_cast<Mat_<Vec3b>>(difference);
          transform(difference_image_16.begin(), difference_image_16.end(), difference_RGB.begin(),
                    [](const short value)// -> Vec3b
                      {
                        const auto abs_value = static_cast<unsigned short>(abs(value));
                        const auto max_value = static_cast<unsigned short>(255);
                        const auto absolute_value = static_cast<unsigned char>(min(abs_value, max_value));
                        return Vec3b(value < 0 ? absolute_value : 0, absolute_value, value > 0 ? absolute_value : 0); //Blue for negative, red for positive, green always to make pixel brighter (BGR order)
                      });
        }
        break;
    }
    return difference;
  }
}
