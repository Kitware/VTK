
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This example demonstrates how to resample an image with vtkImageReslice.
# There are many advantages of vtkImageReslice compareed to vtkImageResample:
# 1) more interpolation modes to choose from (nearest neighbor, linear, cubic)
# 2) you can combine the resampling with a permutation (via the
#    SetResliceAxesDirectionCosines method, see ReslicePermute.tcl)
# 3) you can combine the resampling with a clip or pad operation, via
#    SetOutputExtent and SetOutputOrigin
# 4) you can use SetInformationInput to reslice one data set using the
#    spacing, origin, and extent of another data set

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
reslice SetInterpolationModeToCubic
reslice SetOutputSpacing 0.52 2.2 1.6

# clip out a specific region (uncomment to test)
#  note that 'origin' is given in spacing units, extent in voxel units,
#  if you want to clip with just extent then set origin to 0 0 0, or
#  if you want to center the output then don't call SetOutputOrigin
#reslice SetOutputOrigin 50 10 0
#reslice SetOutputExtent 0 255 0 100 0 10

[reslice GetOutput] UpdateInformation

vtkImageViewer viewer
viewer SetInput [reslice GetOutput]
#[viewer GetImageWindow] DoubleBufferOn
[viewer GetImageWindow] EraseOff
viewer SetZSlice 60
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer Render

#make interface
source ../../imaging/examplesTcl/WindowLevelInterface.tcl

