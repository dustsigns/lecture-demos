Epipolar lines
==============

**Short description**: Illustration of epipolar lines (Illustrates the epipolar lines between two pinhole camera images)

**Author**: Andreas Unterweger

**Status**: Work in progress (features missing)

Overview
--------

![Screenshot](../screenshots/epipolar_lines.png)

Capturing an image of a 3-D object with two pinhole cameras (window *Global view*) produces two images (windows *Left view* and *Right view*). For a known point in the left image, the corresponding point in the right image can be found along the so-called epipolar line under certain conditions. The location and slope of the epipolar line depend on the position of the point in the left image as well as on the relative position and pose of the two cameras with respect to each other.

*Note: The pyramids in the *Global view* window visualize the field of view of the cameras capturing the left (white) and right (grey) images, respectively.*

Usage
-----

Change the known point (red cross) in the left image (see parameters below) to see the position of the epipolar line (red line in the right image) change. Observe that, if the points are visible in both images, like the eye of the bunny for the default program parameters, the corresponding point lies on the epipolar line and is typically not in the same position as the point in the other image (visualized by another red cross).

![Screenshot after selecting a point in the bunny's eye](../screenshots/epipolar_lines_eye.png)

Available actions
-----------------

None. *Note: See below for parameters to change.*

Interactive parameters
----------------------

* **Known point** (mouse over in the *Left view* window): Allows setting the known point in the left image for which the epipolar line in the right image is visualized.

*Note: Additionally, the camera position and zoom in the *Global view* window can be changed using the mouse.*

Program parameters
------------------

* **3-D model**: File path of the PLY model to capture images of.

Hard-coded parameters
---------------------

* `window_width` (local to `struct epipolar_data`): Horizontal size of all windows in pixels.
* `window_height` (local to `struct epipolar_data`): Vertical size of all windows in pixels.
* `camera_x_translation_offset` (local to `GetStereoCameraRotationAndTranslation`): Horizontal distance between the two cameras in relative coordinates.

Known issues
------------

None

Missing features
----------------

* **Include camera rotation**: Apart from the offset (translation) between the two cameras, their relative pose (rotation) impacts the epipolar lines. Thus, a rotation parameter must be added. *Note: Code to add this exists, but is commented as non-zero rotations cause incorrect epipolar lines for unknown reasons.*

License
-------

This demonstration and its documentation (this document) are provided under the 3-Clause BSD License (see [`LICENSE`](../LICENSE) file in the parent folder for details). Please provide appropriate attribution if you use any part of this demonstration or its documentation.