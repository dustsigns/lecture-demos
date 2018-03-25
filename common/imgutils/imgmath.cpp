//Helper functions for calculations on images
// Andreas Unterweger, 2016-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include "math.hpp"

#include "imgmath.hpp"

using namespace std;

using namespace cv;

using namespace comutils;

namespace imgutils
{
  double SAD(const Mat &difference_values)
  {
    assert(!difference_values.empty());
    return norm(difference_values, NORM_L1);
  }

  double SSD(const Mat &difference_values)
  {
    assert(!difference_values.empty());
    const auto sqrt_SSD = norm(difference_values, NORM_L2);
    return sqr(sqrt_SSD); //Square since L2 norm is square root of SSD
  }

  double MSE(const Mat &difference_values)
  {
    assert(difference_values.total() != 0);
    return SSD(difference_values) / difference_values.total();
  }

  double PSNR(const double MSE, const double max_error)
  {
    return GetLevelFromValue(max_error, sqrt(MSE));
  }

  Mat ImageLevelShift(const Mat &image)
  {
    assert(image.type() == CV_8UC1);
    auto &original_image = static_cast<const Mat_<unsigned char>>(image);
    Mat_<double> shifted_image(image.rows, image.cols);
    transform(original_image.begin(), original_image.end(), shifted_image.begin(), LevelShift);
    return shifted_image;
  }

  Mat ReverseImageLevelShift(const Mat &image)
  {
    assert(image.type() == CV_64FC1);
    auto &original_image = static_cast<const Mat_<double>>(image);
    Mat_<unsigned char> shifted_image(image.rows, image.cols);
    transform(original_image.begin(), original_image.end(), shifted_image.begin(), ReverseLevelShift);
    return shifted_image;
  }
  
  Mat GetRaw2DDCTBasisFunctionImage(const unsigned int block_size, const unsigned int i, const unsigned int j, const double amplitude)
  {
    assert(i < block_size && j < block_size);
    assert(amplitude >= 0.0 && amplitude <= 255.0);
    Mat_<double> basis_image(Size(block_size, block_size), 0.0);
    auto scaling_factor = Get2DIDCTCoefficientScalingFactor(block_size, i, j);
    basis_image(i, j) = LevelShift(amplitude) * scaling_factor; //Set one (selected) coefficient to the specified value after level-shifting (with scaling)
    Mat reconstructed_basis_image;
    idct(basis_image, reconstructed_basis_image); //The IDCT of the one coefficient yields the corresponding basis function
    return reconstructed_basis_image;
  }

  Mat Get2DDCTBasisFunctionImage(const unsigned int block_size, const unsigned int i, const unsigned int j, const double amplitude)
  {
    const Mat reconstructed_basis_image = GetRaw2DDCTBasisFunctionImage(block_size, i, j, amplitude);
    return ReverseImageLevelShift(reconstructed_basis_image);
  }
}
