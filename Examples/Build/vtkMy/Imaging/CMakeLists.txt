#
# Source files
#
# Here is where you can add the name of your local imaging classes.
#
set(classes
  vtkImageFoo
)

#
# Create the vtkmyImaging C++ library.
#
vtk_module_add_module(VTKMY::Imaging
  CLASSES ${classes})
