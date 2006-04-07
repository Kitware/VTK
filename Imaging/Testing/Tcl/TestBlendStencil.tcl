package require vtk

# do alpha-blending of two images, but also clip with stencil

vtkBMPReader reader1
reader1 SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

vtkPNMReader reader2
reader2 SetFileName "$VTK_DATA_ROOT/Data/B.pgm"

vtkLookupTable table
table SetTableRange 0 127 
table SetValueRange 0.0 1.0 
table SetSaturationRange 0.0 0.0 
table SetHueRange 0.0 0.0 
table SetAlphaRange 0.9 0.0 
table Build

vtkImageTranslateExtent translate
translate SetInputConnection [reader2 GetOutputPort]
translate SetTranslation 60 60 0

vtkSphere sphere
sphere SetCenter 121 131 0
sphere SetRadius 70

vtkImplicitFunctionToImageStencil functionToStencil
functionToStencil SetInput sphere
[functionToStencil GetOutput] SetUpdateExtent 0 255 0 255 0 0
[functionToStencil GetOutput] Update

vtkImageBlend blend
blend SetInputConnection [reader1 GetOutputPort]
blend AddInputConnection [translate GetOutputPort]
# excercise the ReplaceNthInputConnection method
blend ReplaceNthInputConnection 1 [reader1 GetOutputPort]
blend ReplaceNthInputConnection 1 [translate GetOutputPort]
blend SetOpacity 1 0.8
blend SetStencil [functionToStencil GetOutput]

vtkImageViewer viewer
viewer SetInputConnection [blend GetOutputPort]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 0
viewer Render








