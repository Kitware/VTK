package require vtk


# A script to test the stencil filter.
# removes all but a sphere.


vtkPNGReader reader
reader SetDataSpacing 0.8 0.8 1.5
reader SetDataOrigin  0.0 0.0 0.0
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkSphere sphere
sphere SetCenter 128 128 0
sphere SetRadius 80

vtkImplicitFunctionToImageStencil functionToStencil
functionToStencil SetInput sphere

# test manual updating of stencil
set stencilOriginal [functionToStencil GetOutput]
$stencilOriginal SetSpacing 0.8 0.8 1.5
$stencilOriginal SetOrigin  0.0 0.0 0.0
$stencilOriginal SetUpdateExtent 0 255 0 255 0 0
$stencilOriginal Update

# test making a copying of the stencil (for coverage)
set stencilCopy [$stencilOriginal NewInstance]
$stencilCopy DeepCopy $stencilOriginal

vtkImageShiftScale shiftScale
shiftScale SetInput [reader GetOutput]
shiftScale SetScale 0.2

vtkImageStencil stencil
stencil SetInput [reader GetOutput]
stencil SetBackgroundInput [shiftScale GetOutput]
stencil SetStencil $stencilCopy
$stencilCopy DebugOn
$stencilCopy UnRegister reader

vtkImageViewer viewer
viewer SetInput [stencil GetOutput]
viewer SetZSlice 0
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer Render








