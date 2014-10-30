#
# Source files
#
# Here is where you can add the name of your local imaging classes.
#

set (Imaging_SRCS
  vtkImageFoo.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkmyImaging C++ library
add_library (vtkmyImaging ${Imaging_SRCS})
target_link_libraries(vtkmyImaging vtkmyCommon ${VTK_LIBRARIES})
