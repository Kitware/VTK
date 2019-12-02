#
# Source files
#
# Here is where you can add the name of your local common classes.
#
set(classes
  vtkBar
)

#
# Create the vtkmyCommon C++ library.
#
vtk_module_add_module(VTKMY::Common
  CLASSES ${classes})
