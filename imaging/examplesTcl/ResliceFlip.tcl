
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This example demonstrates how to flip an image with vtkImageReslice.
# The only advantage the vtkImageReslice has over vtkImageFlip is that
# you can permute & flip at the same time (as demonstrated in
# ReslicePermute.tcl).

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetDataSpacing 1 1 2
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
# uncomment this to read all the data at once
#reader Update

vtkImageReslice reslice
reslice SetInput [reader GetOutput]
reslice SetResliceAxesDirectionCosines  -1 0 0   0 +1 0   0 0 +1

vtkImageViewer viewer
viewer SetInput [reslice GetOutput]
#[viewer GetImageWindow] DoubleBufferOn
[viewer GetImageWindow] EraseOff
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer Render

#make interface
source ../../imaging/examplesTcl/WindowLevelInterface.tcl

