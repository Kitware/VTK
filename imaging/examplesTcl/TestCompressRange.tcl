catch {load vtktcl}
# This scripts Compresses the complex components of an image in frequency
# space to view more detail.



source vtkImageInclude.tcl


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

vtkImageCompressRange compress
compress SetInput [fft GetOutput]
compress SetConstant 15

vtkImageViewer viewer
viewer SetInput [compress GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 500
viewer SetColorLevel 0
viewer ColorFlagOn
viewer SetBlueComponent 0


source WindowLevelInterface.tcl








