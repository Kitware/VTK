catch {load vtktcl}
# This scripts shows the real component of an image in frequencey space.


source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT fft
fft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft SetInput [reader GetOutput]
fft ReleaseDataFlagOff
#fft DebugOn

vtkImageFourierCenter center
center SetInput [fft GetOutput]
center SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS

vtkImageViewer viewer
viewer SetInput [center GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 10000
viewer SetColorLevel 4000
viewer ColorFlagOn
viewer SetBlueComponent 0


source WindowLevelInterface.tcl








