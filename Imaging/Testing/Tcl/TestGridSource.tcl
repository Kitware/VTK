package require vtktcl



# add grid lines to an image

vtkImageGridSource imageGrid
imageGrid SetGridSpacing 16 16 0
imageGrid SetGridOrigin 0 0 0
imageGrid SetDataExtent 0 255 0 255 0 0
imageGrid SetDataScalarTypeToUnsignedChar

vtkLookupTable table
table SetTableRange 0 1
table SetValueRange 1.0 0.0 
table SetSaturationRange 0.0 0.0 
table SetHueRange 0.0 0.0 
table SetAlphaRange 0.0 1.0
table Build

vtkImageMapToColors alpha
alpha SetInput [imageGrid GetOutput]
alpha SetLookupTable table

vtkBMPReader reader1
reader1 SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

vtkImageBlend blend
blend SetInput 0 [reader1 GetOutput]
blend SetInput 1 [alpha GetOutput]

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInput [blend GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 0

viewer Render
