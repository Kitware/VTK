#
# Source files
#
# Here is where you can add the name of your local unsorted classes.
#
set(classes
  vtkBar2
)

#
# Create the vtkmyUnsorted C++ library.
#
vtk_module_add_module(VTKMY::Unsorted
  CLASSES ${classes})
