catch {load vtktcl}
# This scripts shows the real component of an image in frequencey space.


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

vtkImageVolume16Reader reader
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT fft
fft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft SetInput [reader GetOutput]
fft ReleaseDataFlagOff
#fft DebugOn

vtkImageViewer viewer
viewer SetInput [fft GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 10000
viewer SetColorLevel 4000
viewer ColorFlagOn
viewer SetBlueComponent 0


source WindowLevelInterface.tcl








