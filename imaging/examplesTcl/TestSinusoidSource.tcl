catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# A script to test the SinusoidSource

source vtkImageInclude.tcl

# Image pipeline

vtkImageSinusoidSource cos
cos SetWholeExtent 0 225 0 225 0 20
cos SetAmplitude 250
cos SetDirection 1 1 1
cos SetPeriod 20
cos ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [cos GetOutput]
viewer SetZSlice 10
viewer SetColorWindow 255
viewer SetColorLevel 127.5
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







