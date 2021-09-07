//3-D visualization window with accompanying configuration window (header)
// Andreas Unterweger, 2017-2021
//This code is licensed under the 3-Clause BSD License. See LICENSE file for details.

#pragma once

#include <map>
#include <functional>

#include <opencv2/core.hpp>
#include <opencv2/viz.hpp>

namespace vizutils
{
  using namespace std;
  
  using namespace cv;
  using namespace cv::viz;

  //Shows a 3-D visualization with a control window
  class ConfigurableVisualization
  {
    public:
      static constexpr auto window_width = 800;
      static constexpr auto window_height = 600;
    
      //Defines a callback function for applying a transform to the viewer pose
      using ViewerTransform = Affine3d (const Affine3d &pose);
      //Defines a callback function when a window control has been changed, e.g., the trackbar value has been changed
      using ControlCallback = void (*)(ConfigurableVisualization &visualization);
      
      //3-D objects to be displayed in the visualization window with their corresponding names
      map<string, Widget3D> objects;
    
      //Constructs a new instance of ConfigurableVisualization with the given names for the visualization and the control window, respectively
      ConfigurableVisualization(const string &visualization_window_name, const string &control_window_name);
      ConfigurableVisualization(const ConfigurableVisualization &original) = delete; //Explicitly delete the copy constructor since duplicating windows is problematic
      ~ConfigurableVisualization();
      
      //Adds a trackbar with the specified callback function, maximum and default value to the control window
      void AddTrackbar(const string &name, ControlCallback callback, const int max_value, const int min_value = 0, const int default_value = 0);
      //Retrieves the value of the trackbar with the specified name
      int GetTrackbarValue(const string &name) const;
      //Updates the value of the trackbar with the specified name, issuing a callback
      void UpdateTrackbarValue(const string &name, const int value);
      
      //Shows the visualization and the control windows, calls the optional transform and initial callback, and waits until a key has been pressed that closes one of them, effectively closing both.
      void ShowWindows(function<ViewerTransform> transform = nullptr, ControlCallback initial_callback = nullptr);
      
      //Returns the visualization's camera object
      Camera GetCamera() const;
      //Replaces the visualization's camera object with the one specified
      void SetCamera(const Camera &camera);
      //Returns the visualization's camera pose
      Affine3d GetViewerPose() const;
      //Replaces the visualization's camera pose with the one specified
      void SetViewerPose(const Affine3d &pose);
      
      //Removes the objects in the visualization window during ShowWindows(). This method does not redraw the window.
      void ClearObjects();
      //Redraws the objects in the visualization window during ShowWindows()
      void RedrawObjects();
    protected:
      //Represents a control element in the control window
      struct WindowControl
      {
        //The function called when the control is changed
        ControlCallback callback;
        //The maximum value associated with the control, e.g., the maximum trackbar value
        int max_parameter;
        //The maximum value associated with the control, e.g., the maximum trackbar value
        int min_parameter;
        //The initial value associated with the control, e.g., the trackbar value
        int default_parameter;
        //The ConfigurableVisualization the WindowControl is associated with
        ConfigurableVisualization &parent;
        
        //Constructs a new instance of WindowControl with the specified callback, minimum, maximum and default value
        WindowControl(ControlCallback callback, const int max_parameter, const int min_parameter, const int default_parameter, ConfigurableVisualization &parent);
      };
    
      const string visualization_window_name;
      const string control_window_name;

      Viz3d visualization;
      
      map<string, WindowControl> controls;
      bool ready;
      
      void ShowVisualizationWindow();
      void ShowControlWindow();
      void AlignWindows(function<ViewerTransform> transform);
  };
}
