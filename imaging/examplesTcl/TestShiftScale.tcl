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

vtkImageShiftScale shiftScale
#shiftScale DebugOn
shiftScale SetInput [reader GetOutput]
shiftScale SetShift -3000.0
shiftScale SetScale -0.12

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [shiftScale GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 256
viewer SetColorLevel 128





#make interface
source WindowLevelInterface.tcl








