catch {load vtktcl}
# Shift and scale an image (in that order)
# This filter is usefull for converting to a lower precision data type.


source vtkImageInclude.tcl


vtkImageReader reader
#reader DebugOn
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#[reader GetOutput] SetMemoryLimit 30

vtkImageShiftScale shiftScale
shiftScale SetInput [reader GetOutput]
shiftScale SetShift 0
shiftScale SetScale 0.5
shiftScale SetOutputScalarTypeToFloat
#[shiftScale GetOutput] SetMemoryLimit 50

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
