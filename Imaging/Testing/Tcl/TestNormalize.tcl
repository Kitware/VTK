package require vtk

# This script is for testing the normalize filter.
# Image pipeline

vtkImageSinusoidSource cos
cos SetWholeExtent 0 225 0 225 0 20
cos SetAmplitude 250
cos SetDirection 1 1 1
cos SetPeriod 20
cos ReleaseDataFlagOff

vtkImageGradient gradient
gradient SetInput [cos GetOutput]
gradient SetDimensionality 3

vtkImageNormalize norm
norm SetInput [gradient GetOutput]

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [norm GetOutput]
viewer SetZSlice 10
viewer SetColorWindow 2.0
viewer SetColorLevel 0

viewer Render
