catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Make an image larger by repeating the data.  Tile.


source vtkImageInclude.tcl


# Image pipeline
vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "$VTK_DATA/earth.ppm"

vtkImageWrapPad pad
pad SetInput [reader GetOutput]
pad SetOutputWholeExtent -100 100 0 250 0 0

vtkImageViewer viewer
viewer SetInput [pad GetOutput]
viewer SetZSlice 0
viewer SetColorWindow 255
viewer SetColorLevel 127
[viewer GetActor2D] SetDisplayPosition 100 0

#make interface
source WindowLevelInterface.tcl







