Andreas Unterweger's Multimedia Lecture Demonstrations
======================================================

This is a collection of demonstrations by [Andreas Unterweger](https://www.andreas-unterweger.eu) originally designed for the lectures *Media Technology* and *Media Informatics* at the Salzburg University of Applied Sciences. These demonstrations allow to interactively explore selected core concepts of audio, image and video processing as well as related topics.

![Screenshot](screenshots/dct_decomposition_5_during_animation.webp)

The above image shows a screenshot of the [2-D DCT decomposition demonstration](image_compression/dct_decomposition_readme.md). A full [list of demonstrations](demolist.md) is available.

Overview
--------

Understanding the basics of multimedia signal processing can be challenging without multimedia-based aids. This [collection of demonstrations](demolist.md) aims at assisting students in deepening their knowledge of selected core concepts by allowing them to interactively explore each concept individually.

Every demonstration (see [full list](demolist.md)) comes with a description on how to use it as well as related background information. While additional information about configuration and customization options are provided, sample files (see below) are included and used by default. This way, the multimedia signal processing concepts can be explored without the need to find or craft adequate samples.

*Note: All demonstrations are functional, but not all demonstrations are complete. Known issues and missing features are listed in the documentation (`readme` files, see below) of the respective demonstration.*

Prerequisites
-------------

These demonstrations require

* A C++17 compiler, e.g., recent versions of *g++*,
* [*OpenCV* 4.8.0](https://github.com/opencv/opencv/archive/4.8.0.zip) with *QT* support and `pkg-config` support as well as the [contributed modules](https://github.com/opencv/opencv_contrib/archive/4.8.0.zip) `viz`, `stitching` and `sfm` included (see [recommended build command](opencv_config.md)),
* [*libao* 1.2.0](http://downloads.xiph.org/releases/ao/libao-1.2.0.zip) with *ALSA* output for all audio-related demonstrations, and
* *make*, *gdb* and *pkg-config*.
* *Xfce* for correct window sizing and positioning due to limitations in *OpenCV*'s and *QT*'s APIs.

All demonstrations have been tested on a [64-bit *Debian* 11.6.0 minimal](https://cdimage.debian.org/debian-cd/current/amd64/iso-cd/debian-11.6.0-amd64-netinst.iso) system with a minimal *Xfce* desktop environment and the following installed packages: `build-essential` (for *g++* 10.2.1 and *make*), `gdb`, `pkg-config`, `cmake` (only required for building *OpenCV*), `qtbase5-dev`, `qtchooser`, `qt5-qmake`, `qtbase5-dev-tools` (for *QT* 5.15.2), `libvtk9-dev` (for *VTK* 9.0.1 which is required for *OpenCV*'s `viz` module), `libeigen3-dev`, `libgflags-dev`, `libgoogle-glog-dev` (all of which are required for *OpenCV*'s `sfm` module) and `libasound2-dev` (for *ALSA*).

Building
--------

Before building, make sure that all prerequisites (see above) are installed on your system. For build parameters, see below.

To build all demonstrations, call `make` from their root directory. To build all demonstrations in a particular folder, e.g., `video_compression`, call `make` from this directory. To build a single demonstration, e.g., the `intra_prediction` demonstration from the `video_compression` folder, call `make` with the name of the demonstration and the postfix `.exe` from its directory, e.g., `make intra_prediction.exe`.

Build parameters (optional)
---------------------------

The following parameters allow changing advanced build options. They are optional and have reasonable defaults.

* **Release mode**: To disable debug builds (which are the default) and enable release builds (with optimizations enabled) instead, set the `DEBUG` flag to `0` when invoking `make`, e.g., `make DEBUG=0`. *Note: The build process does not track debug/release flags of individual files. It is not recommended to build different components with different values of the `DEBUG` flag. Instead, `make clean` should be called in the root folder to clean all intermediate files before switching from debug to release mode or vice versa.*
* **Toolchain**: The file [common/tools.mak](common/tools.mak) specifies variables for all build tools used to build the demonstrations. They can be changed either in this file (not recommended) or by setting the corresponding variables when invoking `make`, e.g., `make CXX=/usr/bin/g++-10`.

Usage
-----

*Note: Most of these demonstrations are designed for screen resolutions of 1680x1050 pixels or larger with standard font and window sizes. Make sure to increase your screen resolution, if necessary, to make all windows fully visible.*

After building all or only selected demonstrations (see above), the generated `.exe` files can be executed directly. Apart from basic usage information when called without (or the wrong number of) parameters, `readme` files are available for all demonstrations, e.g., [video_compression/intra_prediction_readme.md](video_compression/intra_prediction_readme.md) contains a description of the `intra_prediction` demonstration from the `video_compression` folder. A full [list of demonstrations](demolist.md) is available.

For convenience, tailored default parameters and sample files (see below) are defined for each demonstration. They can be invoked via Makefile targets. Call `make` with the prefix `test_` and the name of the demonstration from its directory, e.g., `make test_intra_prediction` to execute the `intra_prediction` demonstration in the `video_compression` directory.

To show-case all demonstrations within a folder, i.e., to execute each of them with its respective tailored default parameters, call `make tests` from their directory. For convenience, the `tests` target is also available in the demonstrations root directory. Call `make tests` there to show-case all demonstrations. Similarly, the target `ordered_tests` is available which show-cases all demonstrations in the exact order in which they are used in the respective lecture. Call `make ordered_tests` to show-case all demonstrations in lecture order.

*Notes: All demonstrations are based on *OpenCV*'s `highgui` module and its *QT*-specific extensions. This means that the demonstration windows **must** be closed by pressing a button on the keyboard. Trying to close the windows using their `x` (close) button will **not** terminate the demonstration. Similarly, controls like buttons, check boxes and radio buttons are not visible in the windows by default, but can only be accessed through the configuration button (at the very right) in the top tool bar. The controls will be shown in a separate window which cannot be used to terminate the demonstration when pressing a button on the keyboard. These usability constraints are specific to *OpenCV* and not the demonstrations.*

Sample files
------------

These demonstrations come with a set of sample files. They are located in the [testdata/](testdata/) directory and grouped into subfolders by type. For each type, a file named `sources.txt` lists the source (URL) for all sample files and optional instructions on how to convert them to the format they are in. If there are no sample files in the [testdata/](testdata/) directory, the instructions can be used to (re-)create them. *Note: Some conversions require special software. For convenience, their Ubuntu package names are specified so that they can be installed before executing the commands.*

License
-------

These demonstrations and their documentation are provided under the [3-Clause BSD License](LICENSE). If you use any part of them, please provide appropriate attribution. For details, see the [`LICENSE`](LICENSE) file. *Note: All data in the `testdata` folder is provided for convenience only and is licensed by third parties.*
