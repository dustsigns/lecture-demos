//Illustration of epipolar lines
// Andreas Unterweger, 2017-2018
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>

#include <opencv2/viz.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/sfm.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include "colors.hpp"
#include "math.hpp"

using namespace std;

using namespace cv;
using namespace viz;
using namespace sfm;

using namespace comutils;
using namespace imgutils;

typedef struct epipolar_data
{
  static constexpr auto global_window_name = "Global view";
  static constexpr auto window_width = 600;
  static constexpr auto window_height = 400;
  
  const string left_window_name;
  const string right_window_name;
  
  Viz3d left_visualization;
  Viz3d right_visualization;
  Viz3d global_visualization;
  
  epipolar_data(const string &left_window_name, const string &right_window_name)
   : left_window_name(left_window_name), right_window_name(right_window_name),
     left_visualization(left_window_name), right_visualization(right_window_name), global_visualization(global_window_name)
  {
    left_visualization.setWindowSize(Size2i(window_width, window_height));
    right_visualization.setWindowSize(Size2i(window_width, window_height));
    global_visualization.setWindowSize(Size2i(window_width, window_height));
  }
} epipolar_data;

static void LoadModel(const char * const filename, Viz3d &visualization)
{
  Mesh mesh = Mesh::load(filename);
  const WMesh visualized_mesh(mesh);
  visualization.showWidget("Object", visualized_mesh);
}

static void ConfigureVisualization(Viz3d &visualization, const char * const model_filename, const bool hide_window = true)
{
  if (hide_window)
    visualization.setOffScreenRendering();
  LoadModel(model_filename, visualization);
}

static Matx33d GetIntrinsicCameraMatrix(const Camera &camera)
{
  const auto focal_length = camera.getFocalLength();
  const auto principal_point = camera.getPrincipalPoint();
  const Matx33d intrinsics(focal_length[0], 0, principal_point[0], 0, focal_length[1], principal_point[1], 0, 0, 1);
  return intrinsics;
}

static void ConfigureGlobalVisualization(epipolar_data &data)
{
  const auto left_camera = data.left_visualization.getCamera();
  const auto left_camera_matrix = GetIntrinsicCameraMatrix(left_camera);
  const auto right_camera = data.right_visualization.getCamera();
  const auto right_camera_matrix = GetIntrinsicCameraMatrix(right_camera);
  WCameraPosition left_camera_object(left_camera_matrix);
  data.global_visualization.showWidget("Left camera", left_camera_object);
  left_camera_object.setPose(data.left_visualization.getViewerPose());
  WCameraPosition right_camera_object(right_camera_matrix, 1.0, Color::gray());
  data.global_visualization.showWidget("Right camera", right_camera_object);
  right_camera_object.setPose(data.right_visualization.getViewerPose());
}

static Affine3d GetStereoCameraRotationAndTranslation()
{
  //constexpr auto camera_y_rotation_angle = 20;
  //constexpr auto camera_z_rotation_angle = 20;
  constexpr auto camera_x_translation_offset = 0.1;
  
  Affine3d pose;
  //pose = pose.rotate(Vec3d(0, DegreesToRadians(camera_y_rotation_angle), 0));
  //pose = pose.rotate(Vec3d(0, 0, DegreesToRadians(camera_z_rotation_angle)));
  pose = pose.translate(Vec3d(camera_x_translation_offset, 0, 0));
  return pose;
}

static void MoveCamera(Viz3d &visualization)
{
  const auto old_pose = visualization.getViewerPose();
  auto pose = old_pose.concatenate(GetStereoCameraRotationAndTranslation());
  visualization.setViewerPose(pose);
}

static void MarkPosition(Mat &image, const int x_position, const int y_position)
{
  const auto marker_color = Red;
  assert(x_position < image.cols && y_position < image.rows);
  drawMarker(image, Point(x_position, y_position), marker_color);
}

static Mat GetFundamentalMatrix(/*const*/ Viz3d &left_visualization, /*const*/ Viz3d &right_visualization)
{
  //TODO: Which of these parameters is/are incorrect? The final result is incorrect when rotation is used.
  const auto left_camera_pose = left_visualization.getViewerPose(); //Method should be const, but is not
  const auto right_camera_pose = right_visualization.getViewerPose(); //Method should be const, but is not
  Mat essential_matrix;
  essentialFromRt(left_camera_pose.rotation(), left_camera_pose.translation(), right_camera_pose.rotation(), right_camera_pose.translation(), essential_matrix);

  const auto left_camera = left_visualization.getCamera();
  const auto left_camera_matrix = GetIntrinsicCameraMatrix(left_camera);
  const auto right_camera = right_visualization.getCamera();
  const auto right_camera_matrix = GetIntrinsicCameraMatrix(right_camera);
  Mat fundamental_matrix;
  fundamentalFromEssential(essential_matrix, left_camera_matrix, right_camera_matrix, fundamental_matrix);
  return fundamental_matrix;
}

static Vec3f ComputeEpipolarLine(/*const*/ epipolar_data &data, const int x_position, const int y_position)
{
  const Mat &fundamental_matrix = GetFundamentalMatrix(data.left_visualization, data.right_visualization);
  const Point2f selected_point(x_position, y_position);
  vector<Vec3f> epipolar_lines;
  computeCorrespondEpilines(vector<Point2f>({selected_point}), 1, fundamental_matrix, epipolar_lines);
  assert(epipolar_lines.size() == 1);
  return epipolar_lines[0];
}

static Point GetLinePointFromLineParameters(const int x_coordinate, const Vec3f &line_parameters)
{
  const auto y = -(line_parameters[2] + line_parameters[0] * x_coordinate) / line_parameters[1];
  return Point(x_coordinate, y);
}

static void DrawEpipolarLine(epipolar_data &data, const int x_position, const int y_position, Mat image)
{
  const auto line_parameters = ComputeEpipolarLine(data, x_position, y_position);
  const Point from = GetLinePointFromLineParameters(0, line_parameters); //Start on the left
  const Point to = GetLinePointFromLineParameters(epipolar_data::window_width, line_parameters); //End on the (far) right
  line(image, from, to, Red, 1);
}

static void RenderViews(/*const*/ epipolar_data &data, const int x_position, const int y_position)
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

Viz3d &ShowWindows(const char * const model_filename)
{
  constexpr auto left_window_name = "Left view";
  namedWindow(left_window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  constexpr auto right_window_name = "Right view";
  namedWindow(right_window_name, WINDOW_GUI_NORMAL | WINDOW_AUTOSIZE);
  static epipolar_data data(left_window_name, right_window_name); //Make variable global so that it is not destroyed after the function returns (for the variable is needed later)
  ConfigureVisualization(data.left_visualization, model_filename);
  ConfigureVisualization(data.right_visualization, model_filename);
  MoveCamera(data.right_visualization);
  ConfigureVisualization(data.global_visualization, model_filename, false);
  ConfigureGlobalVisualization(data);
  RenderViews(data, -1, -1); //Don't select a pixel yet, but render all views (implies imshow)
  setMouseCallback(left_window_name, [](const int event, const int x, const int y, const int, void * const userdata)
                                       {
                                         if (event != EVENT_MOUSEMOVE) //Only react on mouse move
                                           return;
                                         /*const*/ auto &data = *((epipolar_data * const)userdata);
                                         RenderViews(data, x, y);
                                       }, (void*)&data);
  moveWindow(data.left_window_name, 0, 0);
  moveWindow(data.right_window_name, epipolar_data::window_width + 3, 0); //Move window right beside visualization (window size plus 3 border pixels)
  data.global_visualization.spinOnce(1, true);
  data.global_visualization.setWindowPosition(Point2i((epipolar_data::window_width + 3) / 2, epipolar_data::window_height + 50 + 3)); //Move global visualization below left visualization (window size plus additional space for title) and center it
  return data.global_visualization;
}

int main(const int argc, const char * const argv[])
{
  if (argc != 2)
  {
    cout << "Illustrates the epipolar lines between two pinhole camera images." << endl;
    cout << "Usage: " << argv[0] << " <3-D model (PLY) file name>" << endl;
    return 1;
  }
  const auto model_filename = argv[1];
  auto &visualization = ShowWindows(model_filename);
  while (!visualization.wasStopped())
  {
    if (waitKeyEx(1) != -1)
      break;
    visualization.spinOnce(1, true);
  }
  return 0;
}
