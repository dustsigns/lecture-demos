Non-linear distortion
=====================

**Short description**: Illustration of non-linear lense distortion (Illustrates the effect of the distortion vector on a camera image)

**Author**: Andreas Unterweger

**Status**: Complete

Overview
--------

![Screenshot](../screenshots/distortion.png)

Camera lenses cause non-linear distortions in the captured image. Comparing such a distorted image (right part of the *Undistorted vs. distorted* window) with that of an ideal pinhole camera (left) makes the distortions visible. For simplicity, distortions are often modeled as [polynomials with distortion coefficients](https://docs.opencv.org/4.6.0/d9/d0c/group__calib3d.html) to describe radial and tangential distortions.

Usage
-----

Change the distortion coefficients (see parameters below) to see the distorted image change. Observe that negative distortion coefficients change the type of distortion.

![Screenshot after setting negative p1 and p2 coefficients](../screenshots/distortion_negative_p1_p2.png)

Available actions
-----------------

* **Reset** (button): Sets all coefficients to zero (0).

Interactive parameters
----------------------

* **k1*10^(-7)** (track bar in the *Undistorted vs. distorted* window): Allows changing the quadratic radial distortion coefficient between -100 and 100 with a scaling factor of 10^(-7).
* **k2*10^(-10)** (track bar in the *Undistorted vs. distorted* window): Allows changing the quartic (bi-quadratic) radial distortion coefficient between -100 and 100 with a scaling factor of 10^(-10).
* **p1*10^(-5)** (track bar in the *Undistorted vs. distorted* window): Allows changing the quadratic tangential distortion coefficient between -100 and 100 with a scaling factor of 10^(-5).
* **p2*10^(-5)** (track bar in the *Undistorted vs. distorted* window): Allows changing the linear tangential distortion coefficient between -100 and 100 with a scaling factor of 10^(-5).

Program parameters
------------------

* **Input image**: File path of the image to be distorted.

Hard-coded parameters
---------------------

* `max_negative_value` (local to `distortion_data::AddControls`): Absolute value of the minimum coefficient value that can be set via the trackbars
* `max_positive_value` (local to `distortion_data::AddControls`): The maximum coefficient value that can be set via the trackbars

*Note: The scaling factors for the coefficients are hard-coded, but the range of meaningful values is very limited. Thus, it is not recommended to change the scaling factors explicitly.*

Known issues
------------

* **Use of `undistort`**: The `undistort` function is used instead of a `distort` function to calculate the distorted image, potentially showing incorrect results. However, the current implementation illustrates the basic effect of the parameters well enough in principle.

Missing features
----------------

None

License
-------

This demonstration and its documentation (this document) are provided under the 3-Clause BSD License (see [`LICENSE`](../LICENSE) file in the parent folder for details). Please provide appropriate attribution if you use any part of this demonstration or its documentation.
