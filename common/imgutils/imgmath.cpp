//Helper functions for calculations on images
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include "math.hpp"

#include "imgmath.hpp"

namespace imgutils
{
  double SAD(const cv::Mat &difference_values)
  {
    assert(!difference_values.empty());
    return norm(difference_values, cv::NORM_L1);
  }

  double SSD(const cv::Mat &difference_values)
  {
    assert(!difference_values.empty());
    const auto sqrt_SSD = cv::norm(difference_values, cv::NORM_L2);
    return comutils::sqr(sqrt_SSD); //Square since L2 norm is square root of SSD
  }

  double MSE(const cv::Mat &difference_values)
  {
    assert(difference_values.total() != 0);
    return SSD(difference_values) / difference_values.total();
  }

  double PSNR(const double MSE, const double max_error)
  {
    return comutils::GetLevelFromValue(max_error, sqrt(MSE));
  }

  cv::Mat ImageLevelShift(const cv::Mat &image)
  {
    assert(image.type() == CV_8UC1);
    auto &original_image = static_cast<const cv::Mat_<unsigned char>>(image);
    cv::Mat_<double> shifted_image(image.rows, image.cols);
    std::transform(original_image.begin(), original_image.end(), shifted_image.begin(), LevelShift);
    return shifted_image;
  }

  cv::Mat ReverseImageLevelShift(const cv::Mat &image)
  {
    assert(image.type() == CV_64FC1);
    auto &original_image = static_cast<const cv::Mat_<double>>(image);
    cv::Mat_<unsigned char> shifted_image(image.rows, image.cols);
    std::transform(original_image.begin(), original_image.end(), shifted_image.begin(), ReverseLevelShift);
    return shifted_image;
  }
  
  cv::Mat GetRaw2DDCTBasisFunctionImage(const unsigned int block_size, const unsigned int i, const unsigned int j, const double amplitude)
  {
    assert(i < block_size && j < block_size);
    assert(amplitude >= 0.0 && amplitude <= 255.0);
    cv::Mat_<double> basis_image(cv::Size(block_size, block_size), 0.0);
    auto scaling_factor = comutils::Get2DIDCTCoefficientScalingFactor(block_size, i, j);
    basis_image(i, j) = LevelShift(amplitude) * scaling_factor; //Set one (selected) coefficient to the specified value after level-shifting (with scaling)
    cv::Mat reconstructed_basis_image;
    cv::idct(basis_image, reconstructed_basis_image); //The IDCT of the one coefficient yields the corresponding basis function
    return reconstructed_basis_image;
  }

  cv::Mat Get2DDCTBasisFunctionImage(const unsigned int block_size, const unsigned int i, const unsigned int j, const double amplitude)
  {
    const cv::Mat reconstructed_basis_image = GetRaw2DDCTBasisFunctionImage(block_size, i, j, amplitude);
    return ReverseImageLevelShift(reconstructed_basis_image);
  }
}
