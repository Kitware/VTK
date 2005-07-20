package require vtk

# This script shows the magnitude of an image in frequency domain.




# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"



vtkImageCast cast
cast SetInputConnection [reader GetOutputPort]
cast SetOutputScalarTypeToFloat

vtkImageShiftScale scale2
scale2 SetInputConnection [cast GetOutputPort]
scale2 SetScale 0.05

vtkImageGradient gradient
gradient SetInputConnection [scale2 GetOutputPort]
gradient SetDimensionality 3

vtkBMPReader pnm
pnm SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

vtkImageCast cast2
cast2 SetInputConnection [pnm GetOutputPort]
cast2 SetOutputScalarTypeToDouble

vtkImageDotProduct magnitude
magnitude SetInput1 [cast2 GetOutput]
magnitude SetInput2 [gradient GetOutput]

#vtkImageViewer viewer
vtkImageViewer viewer
viewer SetInputConnection [magnitude GetOutputPort]
viewer SetColorWindow 1000
viewer SetColorLevel 300
#viewer DebugOn

viewer Render





