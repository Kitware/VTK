catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# A script to test the Arithmetic filter.
# An image is smoothed then sbutracted from the original image.
# The result is a high-pass filter.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
#reader DebugOn
[reader GetOutput] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

vtkImageShiftScale shiftScale
shiftScale SetInput [cast GetOutput]
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





