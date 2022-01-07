//Image combination functions (template implementations)
// Andreas Unterweger, 2016-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include "common.hpp"

//#include "combine.hpp"

namespace imgutils
{  
  template<size_t N>
  cv::Mat CombineImages(const cv::Mat (&images)[N], const CombinationMode mode, const unsigned int border_size) //Comfort version for initializer lists (no need to explicitly specify N)
  {
	  return CombineImages(N, images, mode, border_size);
  }
}
