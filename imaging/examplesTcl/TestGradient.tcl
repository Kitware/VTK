catch {load vtktcl}
# This script is for testing the 2d Gradient filter.
# It only displays the first component (0) which contains
# the magnitude of the gradient.



source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
gradient ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [gradient GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 400
viewer SetColorLevel 0
viewer ColorFlagOn


#make interface
source WindowLevelInterface.tcl







