package require vtk

# This script shows the magnitude of an image in frequency domain.




# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png



vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

vtkImageShiftScale scale2
scale2 SetInput [cast GetOutput]
scale2 SetScale 0.05

vtkImageGradient gradient
gradient SetInput [scale2 GetOutput]
gradient SetDimensionality 3

vtkBMPReader pnm
pnm SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

vtkImageCast cast2
cast2 SetInput [pnm GetOutput]
cast2 SetOutputScalarTypeToFloat

vtkImageDotProduct magnitude
magnitude SetInput1 [cast2 GetOutput]
magnitude SetInput2 [gradient GetOutput]

#vtkImageViewer viewer
vtkImageViewer viewer
viewer SetInput [magnitude GetOutput]
viewer SetColorWindow 1000
viewer SetColorLevel 300
#viewer DebugOn

viewer Render





