Recommended *OpenCV* configuration
==================================

To build the [demonstrations](demolist.md), the following build configuration of *OpenCV* is recommended. An example build command and a way to verify the configuration follow.

Build command
-------------

When all other required [prerequisites](readmme.md) are installed, the following `cmake` command can be used to build a minimum version of *OpenCV*:

`cmake -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib-3.4.0/modules -D BUILD_LIST=calib3d,features2d,flann,highgui,imgcodecs,imgproc,ml,objdetect,sfm,stereo,video,videoio,viz,xfeatures2d -D BUILD_ITT=OFF -D WITH_1394=OFF -D WITH_GSTREAMER=OFF -D WITH_IPP=OFF -D WITH_JASPER=OFF -D WITH_WEBP=OFF -D WITH_OPENEXR=OFF -D WITH_PVAPI=OFF -D WITH_GIGEAPI=OFF -D WITH_QT=ON -D WITH_TIFF=OFF -D WITH_OPENCL=OFF -D BUILD_opencv_apps=OFF -D BUILD_DOCS=OFF -D BUILD_PACKAGE=OFF -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF -D BUILD_FAT_JAVA_LIB=OFF -D ENABLE_PRECOMPILED_HEADERS=OFF -D CMAKE_BUILD_TYPE=RELEASE ../opencv-3.4.0/`

The paths used in the above command are examples. The command is supposed to be called from within a newly created `build` directory inside the extracted *OpenCV* source code folder. The example paths can be used when the source code for *OpenCV*'s contributed modules has been extracted to the same root directory as *OpenCV*'s source code.

Verification
------------

The output of the `cmake` call (see above for an example) **must** include the following lines (in this order) so that the selected *OpenCV* configuration can be used to build [all demonstrations](demolist.md):

> --   OpenCV modules:
> --     To be built:                 calib3d core features2d flann highgui imgcodecs imgproc ml objdetect sfm stereo video videoio viz xfeatures2d

Additional modules may be listed.

> --   GUI: 
> --     QT:                          YES (ver 5.9.5)
> --       QT OpenGL support:         NO
> --     GTK+:                        NO
> --     VTK support:                 YES (ver 7.1.1)

The listed versions may be different.

> --   Media I/O: 
> --     ZLib:                        /usr/lib/x86_64-linux-gnu/libz.so (ver 1.2.11)
> --     JPEG:                        /usr/lib/x86_64-linux-gnu/libjpeg.so (ver )
> --     PNG:                         /usr/lib/x86_64-linux-gnu/libpng.so (ver 1.6.34)

The listed versions may be different.

> --   Install to:                    /usr/local
