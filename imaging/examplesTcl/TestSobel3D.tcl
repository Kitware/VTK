catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageSobel3D sobel
sobel SetInput [reader GetOutput]
sobel ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [sobel GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 400
viewer SetColorLevel 0


# make interface
source WindowLevelInterface.tcl








