catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageCast imageFloat
  imageFloat SetInput [reader GetOutput]
  imageFloat SetOutputScalarTypeToFloat

vtkImageFlip flipX
  flipX SetInput [imageFloat GetOutput]
  flipX SetFilteredAxes $VTK_IMAGE_X_AXIS 

vtkImageFlip flipY
  flipY SetInput [imageFloat GetOutput]
  flipY SetFilteredAxes $VTK_IMAGE_Y_AXIS 

vtkImageAppend imageAppend
  imageAppend AddInput [imageFloat GetOutput]
  imageAppend AddInput [flipX GetOutput]
  imageAppend AddInput [flipY GetOutput]
  imageAppend SetAppendAxis $VTK_IMAGE_X_AXIS

#flip BypassOn
#flip PreserveImageExtentOn

vtkImageViewer viewer
viewer SetInput [imageAppend GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

#make interface
source WindowLevelInterface.tcl








