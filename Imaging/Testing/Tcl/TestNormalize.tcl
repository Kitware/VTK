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
gradient SetInputConnection [cos GetOutputPort]
gradient SetDimensionality 3

vtkImageNormalize norm
norm SetInputConnection [gradient GetOutputPort]

vtkImageViewer viewer
#viewer DebugOn
viewer SetInputConnection [norm GetOutputPort]
viewer SetZSlice 10
viewer SetColorWindow 2.0
viewer SetColorLevel 0

viewer Render
