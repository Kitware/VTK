catch {load vtktcl}
# This script is for testing the Nd Gaussian Smooth filter.

source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGaussianSmooth smooth
smooth SetInput [reader GetOutput]
smooth SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
smooth SetStandardDeviations 2 10

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [smooth GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl

