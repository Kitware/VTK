package require vtk

# A script to test the stencil filter with a polydata stencil.

# Image pipeline

vtkPNGReader reader
reader SetDataSpacing 0.8 0.8 1.5
reader SetDataOrigin 0.0 0.0 0.0
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkSphereSource sphere
sphere SetPhiResolution 12
sphere SetThetaResolution 12
sphere SetCenter 102 102 0
sphere SetRadius 60

vtkTriangleFilter triangle
triangle SetInputConnection [sphere GetOutputPort]

vtkStripper stripper
stripper SetInputConnection [triangle GetOutputPort]

vtkPolyDataToImageStencil dataToStencil
dataToStencil SetInputConnection [stripper GetOutputPort]
dataToStencil SetOutputSpacing 0.8 0.8 1.5
dataToStencil SetOutputOrigin 0.0 0.0 0.0

vtkImageStencil stencil
stencil SetInputConnection [reader GetOutputPort]
stencil SetStencil [dataToStencil GetOutput]
stencil ReverseStencilOn
stencil SetBackgroundValue 500

vtkImageViewer viewer
viewer SetInputConnection [stencil GetOutputPort]
viewer SetZSlice 0
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer Render








