catch {load vtktcl}
source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGradientMagnitude gradient
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
gradient SetInput [reader GetOutput]
gradient ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [gradient GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl
