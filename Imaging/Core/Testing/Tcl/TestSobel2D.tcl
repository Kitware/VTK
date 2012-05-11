package require vtk

# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.


# Image pipeline
vtkDICOMImageReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/mr.001"

vtkImageSobel2D sobel
sobel SetInputConnection [reader GetOutputPort]
sobel ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInputConnection [sobel GetOutputPort]
viewer SetColorWindow 400
viewer SetColorLevel 0


viewer Render








