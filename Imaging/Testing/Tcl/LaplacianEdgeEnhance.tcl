package require vtk

# This script subtracts the 2D laplacian from an image to enhance the edges.



# Image pipeline
vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageCast cast
cast SetInputConnection [reader GetOutputPort]
cast SetOutputScalarTypeToDouble

vtkImageLaplacian lap
lap SetInputConnection [cast GetOutputPort]
lap SetDimensionality 2

vtkImageMathematics subtract
subtract SetOperationToSubtract
subtract SetInput1 [cast GetOutput]
subtract SetInput2 [lap GetOutput]
subtract ReleaseDataFlagOff
#subtract BypassOn

vtkImageViewer viewer
#viewer DebugOn
viewer SetInputConnection [subtract GetOutputPort]
viewer SetColorWindow 2000
viewer SetColorLevel 1000


viewer Render





