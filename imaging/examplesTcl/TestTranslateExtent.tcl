catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Simple viewer for images.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update

vtkImageTranslateExtent translate
translate SetInput [reader GetOutput]
translate SetTranslation 10 10 10




vtkImageViewer viewer
viewer SetInput [translate GetOutput]
viewer SetZSlice 14
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer Render

#make interface
source WindowLevelInterface.tcl







