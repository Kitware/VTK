catch {load vtktcl}
# This scripts shows a compressed spectrum of an image.


source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT fft
fft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft SetInput [reader GetOutput]
fft ReleaseDataFlagOff
#fft DebugOn

vtkImageMagnitude magnitude
magnitude SetInput [fft GetOutput]
magnitude ReleaseDataFlagOff

vtkImageFourierCenter center
center SetInput [magnitude GetOutput]
center SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS

vtkImageCompressRange compress
compress SetInput [center GetOutput]
compress SetConstant 15

vtkImageViewer viewer
viewer SetInput [compress GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 150
viewer SetColorLevel 170

source WindowLevelInterface.tcl








