catch {load vtktcl}
# This scripts Compresses the complex components of an image in frequency
# space to view more detail.



source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT fft
fft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft SetInput [reader GetOutput]
fft ReleaseDataFlagOff
#fft DebugOn

vtkImageLogarithmicScale compress
compress SetInput [fft GetOutput]
compress SetConstant 15

vtkImageViewer viewer
viewer SetInput [compress GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 500
viewer SetColorLevel 0


source WindowLevelInterface.tcl








