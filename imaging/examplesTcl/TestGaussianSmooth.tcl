catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This script is for testing the Nd Gaussian Smooth filter.

source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGaussianSmooth smooth
smooth SetInput [reader GetOutput]
smooth SetDimensionality 2
smooth SetStandardDeviations 2 10
#smooth SetProgressMethod {set pro [smooth GetProgress]; puts "Completed $pro"; flush stdout}

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [smooth GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl

