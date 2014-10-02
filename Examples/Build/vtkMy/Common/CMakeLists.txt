#
# Source files
#
# Here is where you can add the name of your local common classes.
#

set (Common_SRCS
  vtkBar.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkmyCommon C++ library.
add_library (vtkmyCommon ${Common_SRCS})
target_link_libraries (vtkmyCommon ${VTK_LIBRARIES})
