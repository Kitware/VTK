catch {load vtktcl}
# Divergence measures rate of change of gradient.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageContinuousDilate dilate
dilate SetInput [reader GetOutput]
dilate SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
dilate SetKernelSize 11

vtkImageViewer viewer
viewer SetInput [dilate GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn


source WindowLevelInterface.tcl


