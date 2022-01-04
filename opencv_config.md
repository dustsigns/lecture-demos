Recommended *OpenCV* configuration
==================================

To build the [demonstrations](demolist.md), the following build configuration of *OpenCV* is recommended. An example build command and a way to verify the configuration follow.

Build command
-------------

When all other required [prerequisites](readme.md) are installed, the following `cmake` command can be used to build a minimum version of *OpenCV*:

`cmake -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib-4.5.5/modules -D BUILD_LIST=calib3d,features2d,highgui,imgcodecs,imgproc,objdetect,sfm,stitching,videoio,viz -D WITH_1394=OFF -D WITH_GSTREAMER=OFF -D WITH_IPP=OFF -D WITH_OPENJPEG=OFF -D WITH_JASPER=OFF -D WITH_WEBP=OFF -D WITH_OPENEXR=OFF -D WITH_QT=ON -D WITH_TIFF=OFF -D WITH_OPENCL=OFF -D WITH_LAPACK=OFF -D BUILD_ITT=OFF -D WITH_IMGCODEC_HDR=OFF -D WITH_IMGCODEC_SUNRASTER=OFF -D WITH_IMGCODEC_PFM=OFF -D WITH_QUIRC=OFF -D BUILD_opencv_apps=OFF -D BUILD_DOCS=OFF -D BUILD_PACKAGE=OFF -D BUILD_JAVA=OFF -D OPENCV_GENERATE_PKGCONFIG=ON ../opencv-4.5.5/`

The paths used in the above command are examples. The command is supposed to be called from within a newly created `build` directory inside the extracted *OpenCV* source code folder. The example paths can be used when the source code for *OpenCV*'s contributed modules has been extracted to the same root directory as *OpenCV*'s source code.

Verification
------------

The output of the `cmake` call (see above for an example) **must** include the following lines (in this order) so that the selected *OpenCV* configuration can be used to build [all demonstrations](demolist.md):

````
--   OpenCV modules:
--     To be built:                 calib3d core dnn features2d flann highgui imgcodecs imgproc objdetect sfm stitching videoio viz xfeatures2d
````

Additional modules may be listed.

````
--   GUI: 
--     QT:                          YES (ver 5.15.2)
--       QT OpenGL support:         NO
[...]
--     VTK support:                 YES (ver 9.0.1)
````

The listed versions may be different.

````
--   Media I/O: 
--     ZLib:                        /usr/lib/x86_64-linux-gnu/libz.so (ver 1.2.11)
--     JPEG:                        /usr/lib/x86_64-linux-gnu/libjpeg.so (ver 62)
--     PNG:                         /usr/lib/x86_64-linux-gnu/libpng.so (ver 1.6.37)
[...]
--     PXM:                         YES
````

The listed versions may be different.

````
--   Video I/O:
[...]
--     v4l/v4l2:                    YES (linux/videodev2.h)
````

*Note: V4L support is only needed for accessing webcams and similar devices in those demonstrations which support this. All demonstrations are fully functional with image or video input only, i.e., without webcams. Thus, V4L support is not required.*

````
--   Other third-party libraries:
--     VA:                          NO
--     Eigen:                       YES (ver 3.3.9)
[...]
--     Protobuf:                    build (3.19.1)
````

The listed versions may be different.

````
--   Install to:                    /usr/local
````
