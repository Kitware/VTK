package require vtk

# A script to test the stencil filter with a polydata stencil.

# Image pipeline

vtkPNGReader reader
reader SetDataSpacing 0.8 0.8 1.5
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkSphereSource sphere
sphere SetPhiResolution 12
sphere SetThetaResolution 12
sphere SetCenter 102 102 0
sphere SetRadius 60 

vtkPolyDataToImageStencil dataToStencil
dataToStencil SetInput [sphere GetOutput]

vtkImageStencil stencil
stencil SetInput [reader GetOutput]
stencil SetStencil [dataToStencil GetOutput]
stencil ReverseStencilOn
stencil SetBackgroundValue 500

vtkImageViewer viewer
viewer SetInput [stencil GetOutput]
viewer SetZSlice 0
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer Render








