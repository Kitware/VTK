package require vtk



# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkSimpleImageFilterExample gradient
gradient SetInput [reader GetOutput]

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [gradient GetOutput]
viewer SetColorWindow 1000
viewer SetColorLevel 500


viewer Render
