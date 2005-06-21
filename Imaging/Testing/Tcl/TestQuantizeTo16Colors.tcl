package require vtk

# Make an image larger by repeating the data.  Tile.




# Image pipeline
vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"

vtkImageMirrorPad pad
pad SetInputConnection [reader GetOutputPort]
pad SetOutputWholeExtent -120 320 -120 320 0 0

vtkImageQuantizeRGBToIndex quant
quant SetInputConnection [pad GetOutputPort]
quant SetNumberOfColors 16
quant Update

vtkImageMapToRGBA map
map SetInputConnection [quant GetOutputPort]
map SetLookupTable [quant GetLookupTable]

vtkImageViewer viewer
viewer SetInputConnection [map GetOutputPort]
viewer SetColorWindow 256
viewer SetColorLevel 127
[viewer GetActor2D] SetDisplayPosition 110 110
viewer Render






