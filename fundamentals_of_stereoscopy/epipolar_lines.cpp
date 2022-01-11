//Illustration of epipolar lines
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <algorithm>

#include <opencv2/viz.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/sfm.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include "colors.hpp"
#include "math.hpp"

struct epipolar_data
{
  static constexpr auto global_window_name = "Global view";
  static constexpr auto window_width = 600;
  static constexpr auto window_height = 400;
  
  const std::string left_window_name;
  const std::string right_window_name;
  
  cv::viz::Viz3d left_visualization;
  cv::viz::Viz3d right_visualization;
  cv::viz::Viz3d global_visualization;
  
  epipolar_data(const std::string &left_window_name, const std::string &right_window_name)
   : left_window_name(left_window_name), right_window_name(right_window_name),
     left_visualization(left_window_name), right_visualization(right_window_name), global_visualization(global_window_name)
  {
    left_visualization.setWindowSize(cv::Size2i(window_width, window_height));
    right_visualization.setWindowSize(cv::Size2i(window_width, window_height));
    global_visualization.setWindowSize(cv::Size2i(window_width, window_height));
  }
};

static void LoadModel(const char * const filename, cv::viz::Viz3d &visualization)
{
  const auto mesh = cv::viz::Mesh::load(filename);
  const cv::viz::WMesh visualized_mesh(mesh);
  visualization.showWidget("Object", visualized_mesh);
}

static void ConfigureVisualization(cv::viz::Viz3d &visualization, const char * const model_filename, const bool hide_window = true)
{
  if (hide_window)
    visualization.setOffScreenRendering();
  LoadModel(model_filename, visualization);
}

static cv::Matx33d GetIntrinsicCameraMatrix(const cv::viz::Camera &camera)
{
  const auto focal_length = camera.getFocalLength();
  const auto principal_point = camera.getPrincipalPoint();
  const cv::Matx33d intrinsics(focal_length[0], 0, principal_point[0], 0, focal_length[1], principal_point[1], 0, 0, 1);
  return intrinsics;
}

static void ConfigureGlobalVisualization(epipolar_data &data)
{
  const auto left_camera = data.left_visualization.getCamera();
  const auto left_camera_matrix = GetIntrinsicCameraMatrix(left_camera);
  const auto right_camera = data.right_visualization.getCamera();
  const auto right_camera_matrix = GetIntrinsicCameraMatrix(right_camera);
  cv::viz::WCameraPosition left_camera_object(left_camera_matrix);
  data.global_visualization.showWidget("Left camera", left_camera_object);
  left_camera_object.setPose(data.left_visualization.getViewerPose());
  cv::viz::WCameraPosition right_camera_object(right_camera_matrix, 1.0, cv::viz::Color::gray());
  data.global_visualization.showWidget("Right camera", right_camera_object);
  //TODO: The frustum of the right camera is incorrect (especially visible for only camera_x_translation_offset = 0.3 or camera_y_rotation_angle = 10, respectively)
  right_camera_object.setPose(data.right_visualization.getViewerPose());
}

static cv::Affine3d GetStereoCameraRotationAndTranslation()
{
  //TODO: Create trackbars for all rotations and translations
  constexpr auto camera_x_rotation_angle = 0*10;
  constexpr auto camera_y_rotation_angle = 0*10;
  constexpr auto camera_z_rotation_angle = 0*10; //Do not use this by itself (essential matrix becomes zero)
  constexpr auto camera_x_translation_offset = 0.1;
  constexpr auto camera_y_translation_offset = 0*0.1;
  constexpr auto camera_z_translation_offset = 0*0.1;
  //TODO: This does not work with (non-zero) multi-axes rotations, multi-axes translations as well as combined rotations and translations
  
  cv::Affine3d pose;
  pose = pose.rotate(cv::Vec3d(comutils::DegreesToRadians(camera_x_rotation_angle), 0, 0));
  pose = pose.rotate(cv::Vec3d(0, comutils::DegreesToRadians(camera_y_rotation_angle), 0));
  pose = pose.rotate(cv::Vec3d(0, 0, comutils::DegreesToRadians(camera_z_rotation_angle)));
  const cv::Vec3d translation(camera_x_translation_offset, camera_y_translation_offset, camera_z_translation_offset);
  pose = pose.translate(translation);
  return pose;
}

static void MoveCamera(cv::viz::Viz3d &visualization)
{
  const auto old_pose = visualization.getViewerPose();
  const auto pose_update = GetStereoCameraRotationAndTranslation();
  const cv::Affine3d pose(old_pose.rotation() * pose_update.rotation(), old_pose.translation() + pose_update.translation()); //Update rotation and translation separately
  visualization.setViewerPose(pose);
}

static void MarkPosition(cv::Mat &image, const int x_position, const int y_position)
{
  const auto marker_color = imgutils::Red;
  assert(x_position < image.cols && y_position < image.rows);
  cv::drawMarker(image, cv::Point(x_position, y_position), marker_color);
}

static cv::Mat GetFundamentalMatrix(const cv::viz::Viz3d &left_visualization, const cv::viz::Viz3d &right_visualization)
{
  const auto left_camera_pose = left_visualization.getViewerPose();
  const auto right_camera_pose = right_visualization.getViewerPose();
  cv::Mat essential_matrix;
  cv::sfm::essentialFromRt(left_camera_pose.rotation(), left_camera_pose.translation(), right_camera_pose.rotation(), right_camera_pose.translation(), essential_matrix);

  const auto left_camera = left_visualization.getCamera();
  const auto left_camera_matrix = GetIntrinsicCameraMatrix(left_camera);
  const auto right_camera = right_visualization.getCamera();
  const auto right_camera_matrix = GetIntrinsicCameraMatrix(right_camera);
  cv::Mat fundamental_matrix;
  cv::sfm::fundamentalFromEssential(essential_matrix, left_camera_matrix, right_camera_matrix, fundamental_matrix);
  return fundamental_matrix;
}

static cv::Vec3f ComputeEpipolarLine(const epipolar_data &data, const int x_position, const int y_position)
{
  const cv::Mat &fundamental_matrix = GetFundamentalMatrix(data.left_visualization, data.right_visualization);
  const cv::Point2f selected_point(x_position, y_position);
  std::vector<cv::Vec3f> epipolar_lines;
  cv::computeCorrespondEpilines(std::vector<cv::Point2f>({selected_point}), 1, fundamental_matrix, epipolar_lines);
  assert(epipolar_lines.size() == 1);
  return epipolar_lines[0];
}

static cv::Point GetLinePointFromLineParameters(const int x_coordinate, const cv::Vec3f &line_parameters)
{
  const auto y = -(line_parameters[2] + line_parameters[0] * x_coordinate) / line_parameters[1];
  return cv::Point(x_coordinate, y);
}

static void DrawEpipolarLine(const epipolar_data &data, const int x_position, const int y_position, cv::Mat image)
{
  const auto line_parameters = ComputeEpipolarLine(data, x_position, y_position);
  if (std::all_of(std::begin(line_parameters.val), std::end(line_parameters.val), [](const double value)
                                                                                    {
                                                                                      return std::abs(value) <= std::numeric_limits<double>::epsilon();
                                                                                    }))
  {
    return; //Exit (don't draw anything) if all parameters are zero (essential and fundamental matrix are zero)
  }
  cv::Point from, to;
  if (std::abs(line_parameters[1]) <= std::numeric_limits<float>::epsilon()) //Special case incline zero => vertical line
  {
    from = cv::Point(x_position, 0); //Start on the top
    to = cv::Point(x_position, epipolar_data::window_height); //End on the bottom
  }
  else
  {
    from = GetLinePointFromLineParameters(0, line_parameters); //Start on the left
    to = GetLinePointFromLineParameters(epipolar_data::window_width, line_parameters); //End on the (far) right
  }
  cv::line(image, from, to, imgutils::Red, 1);
}

static void RenderViews(const epipolar_data &data, const int x_position, const int y_position)
{
  const bool annotate = x_position >= 0 && y_position >= 0;
  auto left_view = data.left_visualization.getScreenshot();
  if (annotate)
    MarkPosition(left_view, x_position, y_position);
  cv::imshow(data.left_window_name, left_view);
  auto right_view = data.right_visualization.getScreenshot();
  if (annotate)
  {
    DrawEpipolarLine(data, x_position, y_position, right_view);
    MarkPosition(right_view, x_position, y_position);
  }
  cv::imshow(data.right_window_name, right_view);
}

cv::viz::Viz3d &ShowWindows(const char * const model_filename)
{
  constexpr auto left_window_name = "Left view";
  cv::namedWindow(left_window_name, cv::WINDOW_GUI_NORMAL | cv::WINDOW_AUTOSIZE);
  constexpr auto right_window_name = "Right view";
  cv::namedWindow(right_window_name, cv::WINDOW_GUI_NORMAL | cv::WINDOW_AUTOSIZE);
  static epipolar_data data(left_window_name, right_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  ConfigureVisualization(data.left_visualization, model_filename);
  ConfigureVisualization(data.right_visualization, model_filename);
  MoveCamera(data.right_visualization);
  ConfigureVisualization(data.global_visualization, model_filename, false);
  ConfigureGlobalVisualization(data);
  RenderViews(data, -1, -1); //Don't select a pixel yet, but render all views (implies cv::imshow)
  cv::setMouseCallback(left_window_name, [](const int event, const int x, const int y, const int, void * const userdata)
                                           {
                                             if (event != cv::EVENT_MOUSEMOVE) //Only react on mouse move
                                               return;
                                             auto &data = *(static_cast<const epipolar_data*>(userdata));
                                             RenderViews(data, x, y);
                                           }, static_cast<void*>(&data));
  cv::moveWindow(data.left_window_name, 0, 0);
  cv::moveWindow(data.right_window_name, epipolar_data::window_width + 3, 0); //Move window right beside visualization (window size plus 3 border pixels)
  data.global_visualization.spinOnce(1, true);
  data.global_visualization.setWindowPosition(cv::Point2i((epipolar_data::window_width + 3) / 2, epipolar_data::window_height + 50 + 3)); //Move global visualization below left visualization (window size plus additional space for title) and center it
  return data.global_visualization;
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    std::cout << "Illustrates the epipolar lines between two pinhole camera images." << std::endl;
    std::cout << "Usage: " << argv[0] << " <3-D model (PLY) file name>" << std::endl;
    return 1;
  }
  const auto model_filename = argv[1];
  auto &visualization = ShowWindows(model_filename);
  while (!visualization.wasStopped())
  {
    if (cv::waitKeyEx(1) != -1)
      break;
    visualization.spinOnce(1, true);
  }
  return 0;
}
