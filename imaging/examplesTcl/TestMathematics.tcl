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
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarTypeToFloat

vtkImageShiftScale shiftScale
shiftScale SetInput [reader GetOutput]
shiftScale SetShift 1.0

vtkImageMathematics log
log SetOperationToLog
log SetInput1 [shiftScale GetOutput]
log ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [log GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 4
viewer SetColorLevel 6
viewer Render


# make interface
source WindowLevelInterface.tcl





