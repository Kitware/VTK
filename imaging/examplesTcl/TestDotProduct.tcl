catch {load vtktcl}
# This script shows the magnitude of an image in frequency domain.


source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarType $VTK_FLOAT
#reader DebugOn

vtkImageShiftScale scale2
scale2 SetInput [reader GetOutput]
scale2 SetScale 0.05

vtkImageGradient gradient
gradient SetInput [scale2 GetOutput]
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
gradient ReleaseDataFlagOff
#gradient DebugOn

vtkImageDotProduct magnitude
magnitude SetInput1 [gradient GetOutput]
magnitude SetInput2 [gradient GetOutput]
magnitude ReleaseDataFlagOff

#vtkImageViewer viewer
vtkImageViewer viewer
viewer SetInput [magnitude GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 500
#viewer DebugOn
viewer Render


# make interface
source WindowLevelInterface.tcl





