catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetDataScalarType $VTK_SHORT
#reader DebugOn

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0

vtkImageViewer viewer
viewer SetInput [thresh GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 127.5
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







