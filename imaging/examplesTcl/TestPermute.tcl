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

vtkImagePermute permute
permute SetInput [reader GetOutput]
permute SetFilteredAxes $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS $VTK_IMAGE_X_AXIS

vtkImageViewer viewer
viewer SetInput [permute GetOutput]
viewer SetZSlice 128
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn
#viewer Render

#make interface
source WindowLevelInterface.tcl







