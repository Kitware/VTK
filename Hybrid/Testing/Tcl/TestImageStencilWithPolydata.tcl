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

# test again with a contour
vtkPNGReader reader2
reader2 SetDataSpacing 0.8 0.8 1.5
reader2 SetDataOrigin 0.0 0.0 0.0
reader2 SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkPlane plane
plane SetOrigin 0 0 0
plane SetNormal 0 0 1

vtkCutter cutter
cutter SetInputConnection [sphere GetOutputPort]
cutter SetCutFunction plane

vtkStripper stripper2
stripper2 SetInputConnection [cutter GetOutputPort]

vtkPolyDataToImageStencil dataToStencil2
dataToStencil2 SetInputConnection [stripper2 GetOutputPort]
dataToStencil2 SetOutputSpacing 0.8 0.8 1.5
dataToStencil2 SetOutputOrigin 0.0 0.0 0.0

vtkImageStencil stencil2
stencil2 SetInputConnection [reader2 GetOutputPort]
stencil2 SetStencil [dataToStencil2 GetOutput]
stencil2 SetBackgroundValue 500

vtkImageAppend imageAppend
imageAppend SetInputConnection [stencil GetOutputPort]
imageAppend AddInputConnection [stencil2 GetOutputPort]

vtkImageViewer viewer
viewer SetInputConnection [imageAppend GetOutputPort]
viewer SetZSlice 0
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer Render
