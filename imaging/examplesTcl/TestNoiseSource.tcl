catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# A script to test the NoiseSource

source vtkImageInclude.tcl

# Image pipeline

vtkImageNoiseSource noise
noise SetWholeExtent 0 225 0 225 0 20
noise SetMinimum 0.0
noise SetMaximum 255.0

vtkImageViewer viewer
viewer SetInput [noise GetOutput]
viewer SetZSlice 10
viewer SetColorWindow 255
viewer SetColorLevel 127.5
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







