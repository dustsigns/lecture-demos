//Illustration of epipolar lines
// Andreas Unterweger, 2017-2022
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#include <iostream>
#include <algorithm>
#include <memory>

#include <opencv2/viz.hpp>
#include <opencv2/sfm.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include "common.hpp"
#include "colors.hpp"
#include "math.hpp"
#include "window.hpp"
#include "multiwin.hpp"
#include "confviz.hpp"

class epipolar_data
{
  protected:
    static constexpr auto window_width = 600;
    static constexpr auto window_height = 400;
    
    static constexpr char axes[] = {'x', 'y', 'z'};
    static constexpr char last_axis = axes[comutils::arraysize(axes) - 1];
    static constexpr char default_axis = axes[0];
  
    cv::viz::WMesh model;
  
    cv::viz::Viz3d left_visualization;
    cv::viz::Viz3d right_visualization;
    
    imgutils::Window left_window;
    
    using MouseEventType = imgutils::MouseEvent<epipolar_data&>;
    MouseEventType mouse_event;
    
    imgutils::Window right_window;
    
    imgutils::MultiWindow view_windows;
    
    vizutils::ConfigurableVisualizationWindow global_window;
    std::vector<std::unique_ptr<cv::viz::Widget3D>> objects;
    
    using TrackBarType = imgutils::TrackBar<epipolar_data&>;
    std::unique_ptr<TrackBarType> rotation_trackbars[comutils::arraysize(axes)];
    std::unique_ptr<TrackBarType> translation_trackbars[comutils::arraysize(axes)];
    
    imgutils::MultiWindow all_windows;
    
    cv::Affine3d GetStereoCameraRotationAndTranslation()
    {
      cv::Affine3d pose;
      for (size_t i = 0; i < comutils::arraysize(rotation_trackbars); i++)
      {
        const auto &trackbar = rotation_trackbars[i];
        const auto rotation_degrees = trackbar->GetValue();
        const auto rotation = comutils::DegreesToRadians(rotation_degrees);
        cv::Vec3d rotation_vector;
        rotation_vector[i] = rotation;
        pose = pose.rotate(rotation_vector);
      }
      cv::Vec3d translation_vector;
      for (size_t i = 0; i < comutils::arraysize(translation_trackbars); i++)
      {
        const auto &trackbar = translation_trackbars[i];
        const auto translation_unscaled = trackbar->GetValue();
        const auto translation = translation_unscaled * 0.01;
        translation_vector[i] = translation;
      }
      pose = pose.translate(translation_vector);
      return pose; //TODO: This does not work with (non-zero) multi-axes rotations, multi-axes translations as well as combined rotations and translations
    }

    void MoveCamera()
    {
      const auto old_pose = left_visualization.getViewerPose();
      const auto pose_update = GetStereoCameraRotationAndTranslation();
      const cv::Affine3d pose(old_pose.rotation() * pose_update.rotation(), old_pose.translation() + pose_update.translation()); //Update rotation and translation separately
      right_visualization.setViewerPose(pose);
    }
    
    static cv::Matx33d GetIntrinsicCameraMatrix(const cv::viz::Camera &camera)
    {
      const auto focal_length = camera.getFocalLength();
      const auto principal_point = camera.getPrincipalPoint();
      const cv::Matx33d intrinsics(focal_length[0], 0, principal_point[0], 0, focal_length[1], principal_point[1], 0, 0, 1);
      return intrinsics;
    }

    void ConfigureGlobalVisualization()
    {
      auto &window = global_window.visualization_window;
      objects.clear();
      window.ClearWidgets();
      window.AddWidget("Model", &model);
      
      const auto left_camera = left_visualization.getCamera();
      const auto left_camera_matrix = GetIntrinsicCameraMatrix(left_camera);
      auto left_camera_object = std::make_unique<cv::viz::WCameraPosition>(left_camera_matrix);
      window.AddWidget("Left camera", left_camera_object.get());
      left_camera_object->setPose(left_visualization.getViewerPose());
      objects.emplace_back(std::move(left_camera_object));
      
      const auto right_camera = right_visualization.getCamera();
      const auto right_camera_matrix = GetIntrinsicCameraMatrix(right_camera);
      auto right_camera_object = std::make_unique<cv::viz::WCameraPosition>(right_camera_matrix, 1.0, cv::viz::Color::gray());
      window.AddWidget("Right camera", right_camera_object.get());
      right_camera_object->setPose(right_visualization.getViewerPose());
      objects.emplace_back(std::move(right_camera_object));
      //TODO: The frustum of the right camera is incorrect (especially visible for only camera_x_translation_offset = 0.3 or camera_y_rotation_angle = 10, respectively)
    }
    
    static void MarkPosition(cv::Mat &image, const int x_position, const int y_position)
    {
      const auto marker_color = imgutils::Red;
      assert(x_position < image.cols && y_position < image.rows);
      cv::drawMarker(image, cv::Point(x_position, y_position), marker_color);
    }

    cv::Mat GetFundamentalMatrixFromVisualizations()
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

    cv::Vec3f ComputeEpipolarLine(const int x_position, const int y_position)
    {
      const cv::Mat &fundamental_matrix = GetFundamentalMatrixFromVisualizations();
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

    void DrawEpipolarLine(const int x_position, const int y_position, cv::Mat image)
    {
      const auto line_parameters = ComputeEpipolarLine(x_position, y_position);
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
        to = cv::Point(x_position, window_height); //End on the bottom
      }
      else
      {
        from = GetLinePointFromLineParameters(0, line_parameters); //Start on the left
        to = GetLinePointFromLineParameters(window_width, line_parameters); //End on the (far) right
      }
      cv::line(image, from, to, imgutils::Red, 1);
    }
    
    void UpdateImages(const int x_position = -1, const int y_position = -1)
    {
      const bool annotate = x_position >= 0 && y_position >= 0;
      auto left_view = left_visualization.getScreenshot();
      if (annotate)
        MarkPosition(left_view, x_position, y_position);
      left_window.UpdateContent(left_view);
      MoveCamera(); //Update camera in right visualization
      auto right_view = right_visualization.getScreenshot();
      if (annotate)
      {
        DrawEpipolarLine(x_position, y_position, right_view);
        MarkPosition(right_view, x_position, y_position);
      }
      right_window.UpdateContent(right_view);
      ConfigureGlobalVisualization(); //Update cameras in global view
    }
    
    static void MouseEvent(const int event, const int x, const int y, epipolar_data &data)
    {
      if (event != cv::EVENT_MOUSEMOVE) //Only react on mouse move
        return;
      data.UpdateImages(x, y);
    }
    
    static void UpdateAllImages(epipolar_data &data)
    {
      data.UpdateImages();
    }
    
    void AddControls()
    {
      std::transform(std::begin(axes), std::end(axes), std::begin(rotation_trackbars),
                     [this](const char axis)
                           {
                             using namespace std::string_literals;
                             const auto trackbar_name = "Rotation ("s + axis + ") [°]";
                             return std::make_unique<TrackBarType>(trackbar_name, global_window.configuration_window, 360, 0, 0, UpdateAllImages, *this);
                           });
      std::transform(std::begin(axes), std::end(axes), std::begin(translation_trackbars),
                     [this](const char axis)
                           {
                             using namespace std::string_literals;
                             const auto trackbar_name = "Translation ("s + axis + ") [px]";
                             const auto limit = axis == last_axis ? 500: 50; //Different range for z axis
                             const auto default_value = axis == default_axis ? 10 : 0; //Translation with x=10
                             return std::make_unique<TrackBarType>(trackbar_name, global_window.configuration_window, limit, -limit, default_value, UpdateAllImages, *this);
                           });
    }
    
    static constexpr auto left_window_name = "Left camera view";
    static constexpr auto right_window_name = "Right camera view";
    static constexpr auto global_window_name = "Global view";
    static constexpr auto global_configuration_window_name = "Relative camera pose configuration";
  public:
    epipolar_data(const char * const model_filename)
     : model(cv::viz::WMesh(cv::viz::Mesh::load(model_filename))),
       left_visualization(left_window_name),
       right_visualization(right_window_name),
       left_window(left_window_name),
       mouse_event(left_window, MouseEvent, *this),
       right_window(right_window_name),
       view_windows({&left_window, &right_window}, imgutils::WindowAlignment::Horizontal),
       global_window(global_window_name, global_configuration_window_name, imgutils::WindowAlignment::Horizontal),
       all_windows({&view_windows, &global_window}, imgutils::WindowAlignment::Vertical)
    {
      AddControls();
      const cv::Size window_size(window_width, window_height);
      left_visualization.setOffScreenRendering();
      left_visualization.setWindowSize(window_size);
      left_visualization.showWidget("Model", model);
      
      right_visualization.setOffScreenRendering();
      right_visualization.setWindowSize(window_size);
      right_visualization.showWidget("Model", model);
      global_window.SetSize(window_size);
      ConfigureGlobalVisualization(); //Pre-load global visualization to initialize camera
    }
    
    void ShowImages()
    {
      all_windows.ShowInteractive([this]()
                                        {
                                          UpdateImages(); //Update with default values
                                          all_windows.Update(); //Realign windows
                                        });
    }
};

void ShowWindows(const char * const model_filename)
{
  epipolar_data data(model_filename);
  data.ShowImages();
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
  ShowWindows(model_filename);
  return 0;
}
