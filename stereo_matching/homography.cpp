//Illustration of plane-to-plane warping with a homography
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "colors.hpp"
#include "window.hpp"
#include "multiwin.hpp"

class homography_data
{
  protected:
    imgutils::Window source_window;
    
    imgutils::Window target_window;
    
    using ButtonType = imgutils::Button<homography_data&>;
    ButtonType clear_button;
    
    using MouseEventType = imgutils::MouseEvent<homography_data&>;
    MouseEventType target_mouse_event;
    
    imgutils::MultiWindow all_windows;
  
    const cv::Mat source_image;
    const cv::Mat target_image;
    
    std::vector<cv::Point2f> target_corners;
  
    cv::Mat DrawCorners()
    {
      constexpr auto circle_radius = 3;
      const auto circle_color = imgutils::Red;
      
      cv::Mat new_image(target_image.clone()); //Copy original image
      for (const auto &corner : target_corners)
        circle(new_image, corner, circle_radius, circle_color, -1); //Filled circle
      return new_image;
    }
    
    cv::Mat WarpImage()
    {
      assert(target_corners.size() == 4);
      
      const auto max_x = source_image.cols - 1;
      const auto max_y = source_image.rows - 1;
      const cv::Point2f top_left(0, 0);
      const cv::Point2f top_right(max_x, 0);
      const cv::Point2f bottom_right(max_x, max_y);
      const cv::Point2f bottom_left(0, max_y);
      const std::vector<cv::Point2f> source_corners({ top_left, top_right, bottom_right, bottom_left });
      
      const auto homography = getPerspectiveTransform(source_corners, target_corners);
      cv::Mat warped_image;
      cv::warpPerspective(source_image, warped_image, homography, target_image.size());
      
      const cv::Mat image_mask = cv::Mat(source_image.size(), CV_8UC1, cv::Scalar(255)); //Create mask
      cv::Mat image_mask_warped;
      cv::warpPerspective(image_mask, image_mask_warped, homography, target_image.size()); //Warp mask so that unused pixels become zero
      cv::Mat new_image(target_image.clone()); //Copy original image
      warped_image.copyTo(new_image, image_mask_warped); //Copy warped image into unused pixels
      return new_image;
    }
  
    static void UpdateImage(homography_data &data)
    {
      if (data.target_corners.size() == 4) //TODO: Make this more intuitive
      {
        const auto warped_image = data.WarpImage();
        data.target_window.UpdateContent(warped_image);
      }
      else
      {
        const auto corner_image = data.DrawCorners();
        data.target_window.UpdateContent(corner_image);
      }
    }
    
    static void ClearSelection(homography_data &data)
    {
      data.target_corners.clear();
      UpdateImage(data);
    }
    
    static void TargetMouseEvent(const int event, const int x, const int y, homography_data &data)
    {
      if (event != cv::EVENT_LBUTTONDOWN) //Only react on mouse down
        return;
      if (data.target_corners.size() < 4)
        data.target_corners.push_back(cv::Point2f(x, y));
      UpdateImage(data);
    }
  
    static constexpr auto source_window_name = "Source image";
    
    static constexpr auto target_window_name = "Target image";
    static constexpr auto clear_button_name = "Clear selection";
  public:
    homography_data(const cv::Mat &source_image, const cv::Mat &target_image)
     : source_window(source_window_name, source_image),
       target_window(target_window_name, target_image),
       clear_button(clear_button_name, target_window, ClearSelection, *this),
       target_mouse_event(target_window, TargetMouseEvent, *this),
       all_windows({&source_window, &target_window}, imgutils::WindowAlignment::Horizontal),
       source_image(source_image), target_image(target_image)
    {
      source_window.SetPositionLikeEnhanced(); //Position this window aligned with the target (enhanced) one
      UpdateImage(*this); //Update with default values
    }
    
    void ShowImages()
    {
      all_windows.ShowInteractive();
    }
};

static void ShowImages(const cv::Mat &source_image, const cv::Mat &target_image)
{
  homography_data data(source_image, target_image);
  data.ShowImages();
}

int main(const int argc, const char * const argv[])
{
  if (argc != 3)
  {
    std::cout << "Illustrates the warp in perspective from one plane to another." << std::endl;
    std::cout << "Usage: " << argv[0] << " <source image> <target image>" << std::endl;
    return 1;
  }
  const auto source_image_filename = argv[1];
  const cv::Mat source_image = cv::imread(source_image_filename);
  if (source_image.empty())
  {
    std::cerr << "Could not read source image '" << source_image_filename << "'" << std::endl;
    return 2;
  }
  const auto target_image_filename = argv[2];
  const cv::Mat target_image = cv::imread(target_image_filename);
  if (target_image.empty())
  {
    std::cerr << "Could not read target image '" << target_image_filename << "'" << std::endl;
    return 3;
  }
  ShowImages(source_image, target_image);
  return 0;
}
