catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Simple viewer for images.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update

vtkImageClip clip
clip SetInput [reader GetOutput]
clip SetOutputWholeExtent 0 255 50 150 1 93
clip ClipDataOn
clip ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetSize 256 256
viewer SetInput [clip GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn

#make interface
source WindowLevelInterface.tcl







