catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


vtkTIFFReader reader
reader SetFileName "$VTK_DATA/testTIFF.tif"
#reader SetFileName $argv

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [reader GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5

#make interface
source WindowLevelInterface.tcl


