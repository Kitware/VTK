catch {load vtktcl}

source ../../imaging/examplesTcl/vtkImageInclude.tcl

# Image pipeline

vtkImageBlockReader reader
  reader SetFilePattern "tmp/blocks_%d_%d_%d.vtk"
  reader SetDivisions 4 4 4 
  reader SetOverlap 3
  reader SetWholeExtent 0 255 0 255 1 33
  reader SetNumberOfScalarComponents 1
  reader SetScalarType $VTK_UNSIGNED_SHORT

vtkImageViewer viewer
  viewer SetInput [reader GetOutput]
  viewer SetZSlice 14
  viewer SetColorWindow 2000
  viewer SetColorLevel 1000
  viewer SetPosition 50 50
  viewer Render


#make interface
source ../../imaging/examplesTcl/WindowLevelInterface.tcl







