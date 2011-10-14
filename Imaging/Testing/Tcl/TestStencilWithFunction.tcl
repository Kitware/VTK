package require vtk


# A script to test the stencil filter.
# removes all but a sphere.


vtkPNGReader reader
reader SetDataSpacing 0.8 0.8 1.5
reader SetDataOrigin  0.0 0.0 0.0
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"
reader Update

vtkSphere sphere
sphere SetCenter 128 128 0
sphere SetRadius 80

vtkImplicitFunctionToImageStencil functionToStencil
functionToStencil SetInput sphere
functionToStencil SetInformationInput [reader GetOutput]
functionToStencil Update

# test making a copying of the stencil (for coverage)
set stencilOriginal [functionToStencil GetOutput]
set stencilCopy [$stencilOriginal NewInstance]
$stencilCopy DeepCopy [functionToStencil GetOutput]

vtkImageShiftScale shiftScale
shiftScale SetInputConnection [reader GetOutputPort]
shiftScale SetScale 0.2
shiftScale Update

vtkImageStencil stencil
stencil SetInputConnection [reader GetOutputPort]
stencil SetBackgroundInputData [shiftScale GetOutput]
stencil SetStencilData $stencilCopy
$stencilCopy UnRegister stencil

vtkImageViewer viewer
viewer SetInputConnection [stencil GetOutputPort]
viewer SetZSlice 0
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer Render








