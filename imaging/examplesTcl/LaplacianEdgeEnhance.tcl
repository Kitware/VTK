catch {load vtktcl}
# This script subtracts the 2D laplacian from an image to enhance the edges.


source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarType $VTK_FLOAT

vtkImageLaplacian lap
lap SetInput [reader GetOutput]
lap SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS

vtkImageArithmetic subtract
subtract SetOperatorToSubtract
subtract SetInput1 [reader GetOutput]
subtract SetInput2 [lap GetOutput]
subtract ReleaseDataFlagOff
#subtract BypassOn

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [subtract GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl





