#
# Source files
#
# Here is where you can add the name of your local unsorted classes.
#

set (Unsorted_SRCS
  vtkBar2.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkmyUnsorted C++ library.
add_library (vtkmyUnsorted ${Unsorted_SRCS})
target_link_libraries (vtkmyUnsorted ${VTK_LIBRARIES})
