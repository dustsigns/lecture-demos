//Illustration of 2-D DCT basis functions
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "math.hpp"
#include "imgmath.hpp"
#include "combine.hpp"
#include "window.hpp"

static cv::Mat GenerateBasisFunctions(const size_t block_size, const size_t i)
{
  std::vector<cv::Mat> basis_functions(block_size);
  for (size_t j = 0; j < block_size; j++)
    basis_functions[j] = imgutils::Get2DDCTBasisFunctionImage(block_size, i, j);
  return imgutils::CombineImages(block_size, basis_functions.data(), imgutils::CombinationMode::Horizontal, 1);
}

static cv::Mat GenerateBasisFunctions(const size_t block_size)
{
  std::vector<cv::Mat> basis_function_rows(block_size);
  for (size_t i = 0; i < block_size; i++)
    basis_function_rows[i] = GenerateBasisFunctions(block_size, i);
  return imgutils::CombineImages(block_size, basis_function_rows.data(), imgutils::CombinationMode::Vertical, 1);
}

static void ShowBasisFunctions(const unsigned int block_size)
{
  constexpr auto window_size = 500;
  const auto window_name = std::to_string(block_size) + "x" + std::to_string(block_size) + "-DCT basis functions";
  const cv::Mat basis_functions = GenerateBasisFunctions(block_size);
  imgutils::Window window(window_name, basis_functions, cv::Size(window_size, window_size));
  window.ShowInteractive();
}

int main(const int argc, const char * const argv[])
{
  if (argc > 2)
  {
    std::cout << "Illustrates the basis functions of the 2-D-DCT." << std::endl;
    std::cout << "Usage: " << argv[0] << " [<DCT block size> = 8]" << std::endl;
    return 1;
  }
  unsigned int block_size = 8;
  if (argc == 2)
  {
    block_size = std::stoi(argv[1]);
    if (block_size < 1 || block_size > 32)
    {
      std::cerr << "DCT block size must be between 1 and 32" << std::endl;
      return 2;
    }
  }
  ShowBasisFunctions(block_size);
  return 0;
}
