catch {load vtktcl}
# A script to test the Arithmetic filter.
# An image is smoothed then sbutracted from the original image.
# The result is a high-pass filter.


source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGaussianSmooth smooth
smooth SetInput [reader GetOutput]
smooth SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
smooth SetStandardDeviation 6.0

vtkImageArithmetic subtract
subtract SetOperatorToSubtract
subtract SetInput1 [reader GetOutput]
subtract SetInput2 [smooth GetOutput]
subtract ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [subtract GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1500
viewer SetColorLevel 0
viewer Render


# make interface
source WindowLevelInterface.tcl





