catch {load vtktcl}
# Test the type conversion of caches.


set sliceNumber 22

set VTK_FLOAT              1
set VTK_INT                2
set VTK_SHORT              3
set VTK_UNSIGNED_SHORT     4
set VTK_UNSIGNED_CHAR      5

set VTK_IMAGE_X_AXIS             0
set VTK_IMAGE_Y_AXIS             1
set VTK_IMAGE_Z_AXIS             2
set VTK_IMAGE_TIME_AXIS          3
set VTK_IMAGE_COMPONENT_AXIS     4




# Image pipeline

vtkImageSeriesReader reader
reader ReleaseDataFlagOff
reader SwapBytesOn
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../data/fullHead/headsq"
reader SetPixelMask 0x7fff
reader SetOutputScalarType $VTK_SHORT
reader DebugOn

vtkImageRegion region
region SetScalarType $VTK_UNSIGNED_CHAR
region SetExtent 0 255 0 255 22 22

puts [region Print]

[reader GetOutput] UpdateRegion region


vtkImageXViewer viewer
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
viewer SetInput [region GetOutput]
viewer SetCoordinate2 $sliceNumber
viewer SetColorWindow 3000
viewer SetColorLevel 1500
#viewer DebugOn
viewer Render






