catch {load vtktcl}
# Make an image larger by repeating the data.  Tile.


source vtkImageInclude.tcl


# Image pipeline
vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "../../../vtkdata/earth.ppm"

vtkImageMirrorPad pad
pad SetInput [reader GetOutput]
pad SetOutputWholeExtent -220 340 -220 340 0 0

vtkImageQuantizeRGBToIndex quant
quant SetInput [pad GetOutput]
quant SetNumberOfColors 16
quant Update

vtkImageMapToRGBA map
map SetInput [quant GetOutput]
map SetLookupTable [quant GetLookupTable]

vtkImageViewer viewer
viewer SetInput [map GetOutput]
viewer SetZSlice 0
viewer SetColorWindow 256
viewer SetColorLevel 127
[viewer GetActor2D] SetDisplayPosition 220 220
viewer Render

wm withdraw .






