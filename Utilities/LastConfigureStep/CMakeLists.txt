# The commands in this directory are intended to be executed as
# the end of the whole configuration process, as a "last step".
# This directory is typically the last SUBDIRS in the main CMakeLists.txt.
# It enable the above commands to use variables that might have been configured
# in previous SUBDIRS. This is especially important when it comes to
# the CONFIGURE_FILE command, since in IMMEDIATE mode that command will
# use the current values of CMake variables instead of waiting until the
# end of CMakeLists processing, i.e. instead of waiting until some variables
# are configured in SUBDIRS.

#-----------------------------------------------------------------------------
# Create the VTKConfig.cmake file containing the VTK configuration.
# Since it might generate configuration file depending
INCLUDE(${VTK_SOURCE_DIR}/vtkGenerateVTKConfig.cmake)
