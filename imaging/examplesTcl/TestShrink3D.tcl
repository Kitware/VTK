# Halves the size of the image in the x, Y and Z dimensions.

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageShrink3D shrink
shrink SetInput [reader GetOutput]
shrink SetShrinkFactors 2 2 2
shrink AveragingOff
#shrink SetProgressMethod {set pro [shrink GetProgress]; puts "Completed $pro"; flush stdout}
#shrink Update
shrink SetNumberOfThreads 1

vtkImageViewer viewer
viewer SetInput [shrink GetOutput]
viewer SetZSlice 11
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl







