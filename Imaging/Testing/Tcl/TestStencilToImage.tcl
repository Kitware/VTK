package require vtk


# A script to test converting a stencil to a binary image


vtkSphere sphere
sphere SetCenter 128 128 0
sphere SetRadius 80

vtkImplicitFunctionToImageStencil functionToStencil
functionToStencil SetInput sphere
functionToStencil SetOutputOrigin 0 0 0
functionToStencil SetOutputSpacing 1 1 1
functionToStencil SetOutputWholeExtent 0 255 0 255 0 0

vtkImageStencilToImage stencilToImage
stencilToImage SetInputConnection [functionToStencil GetOutputPort]
stencilToImage SetOutsideValue 0
stencilToImage SetInsideValue 255

vtkImageViewer viewer
viewer SetInputConnection [stencilToImage GetOutputPort]
viewer SetZSlice 0
viewer SetColorWindow 255
viewer SetColorLevel 127.5
viewer Render
