#-----------------------------------------------------------------------------
# vtkmpeg2encode
#-----------------------------------------------------------------------------

 This source tree for the mpeg2 library is designed to be built as a
 standalone library using CMake. This is an implementation specifically
 designed to link into VTK. It is not included directly with any VTK
 distributions because VTK has a "patent free" policy which prohibits
 including patented code in the VTK source tree...

 First, build the vtkMPEG2Encode library using CMake to configure the build.
 Then, assuming you are willing to take on the responsibility of including
 patented code in your software project, you can turn on the
 VTK_USE_MPEG2_ENCODER option and link vtkMPEG2Encode into your build of VTK
 to enable the vtkMPEG2Writer class.

 See the comments at the top of the source code files for the disclaimer of
 warranty and comments regarding royalty payments to MPEG patent holders.

 The following comments are copied from VTK's main CMakeLists.txt file -
 these comments appear just prior to the definition of the
 VTK_USE_MPEG2_ENCODER CMake option.

#-----------------------------------------------------------------------------
# MPEG2
#
# Portions of the mpeg2 library are patented. VTK does not enable linking to
# this library by default so VTK can remain "patent free". Users who wish to
# link in mpeg2 functionality must build that library separately and then
# turn on VTK_USE_MPEG2_ENCODER when configuring VTK. After turning on
# VTK_USE_MPEG2_ENCODER, you must also set the CMake variables
# vtkMPEG2Encode_INCLUDE_PATH and vtkMPEG2Encode_LIBRARIES.
#
# To use the patented mpeg2 library, first build it, then set the following
# CMake variables during the VTK configure step:
#   VTK_USE_MPEG2_ENCODER = ON
#   vtkMPEG2Encode_INCLUDE_PATH = /path/to/vtkmpeg2encode;/path/to/vtkmpeg2encode-bin
#   vtkMPEG2Encode_LIBRARIES = /path/to/vtkmpeg2encode-bin/vtkMPEG2Encode.lib
#
# Or using -D args on the cmake/ccmake command line:
#   -DVTK_USE_MPEG2_ENCODER:BOOL=ON
#   "-DvtkMPEG2Encode_INCLUDE_PATH:PATH=/path/to/vtkmpeg2encode;/path/to/vtkmpeg2encode-bin"
#   "-DvtkMPEG2Encode_LIBRARIES:STRING=/path/to/vtkmpeg2encode-bin/vtkMPEG2Encode.lib"
#
# You are solely responsible for any legal issues associated with using
# patented code in your software.
#
#-----------------------------------------------------------------------------
