catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Shift and scale an image (in that order)
# This filter is usefull for converting to a lower precision data type.


source vtkImageInclude.tcl


vtkImageReader reader
#reader DebugOn
[reader GetOutput] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
[reader GetOutput] SetMemoryLimit 30

vtkImageShiftScale shiftScale
shiftScale SetInput [reader GetOutput]
shiftScale SetShift 0
shiftScale SetScale 0.5
shiftScale SetOutputScalarTypeToFloat
[shiftScale GetOutput] SetMemoryLimit 50

vtkImageShiftScale shiftScale2
shiftScale2 SetInput [shiftScale GetOutput]
shiftScale2 SetShift 0
shiftScale2 SetScale 2.0

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [shiftScale2 GetOutput]
#viewer SetInput [reader GetOutput]
viewer SetColorWindow 1024
viewer SetColorLevel 512

#make interface
source WindowLevelInterface.tcl

vtkPNMWriter w
w SetFileName D:/vtknew/vtk/graphics/examplesTcl/mace2.ppm
w SetInput [shiftScale GetOutput]
#w Write
