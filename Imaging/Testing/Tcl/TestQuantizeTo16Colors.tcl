package require vtk

# Make an image larger by repeating the data.  Tile.




# Image pipeline
vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"

vtkImageMirrorPad pad
pad SetInput [reader GetOutput]
pad SetOutputWholeExtent -120 320 -120 320 0 0

vtkImageQuantizeRGBToIndex quant
quant SetInput [pad GetOutput]
quant SetNumberOfColors 16
quant Update

vtkImageMapToRGBA map
map SetInput [quant GetOutput]
map SetLookupTable [quant GetLookupTable]

vtkImageViewer viewer
viewer SetInput [map GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127
[viewer GetActor2D] SetDisplayPosition 110 110
viewer Render






