package require vtktcl



# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageGradientMagnitude gradient
gradient SetDimensionality 2
gradient SetInput [reader GetOutput]

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [gradient GetOutput]
viewer SetColorWindow 1000
viewer SetColorLevel 500


viewer Render
