catch {load vtktcl}
source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageCast imageFloat
  imageFloat SetInput [reader GetOutput]
  imageFloat SetOutputScalarTypeToFloat

vtkImageFlip flipNO
  flipNO SetInput [imageFloat GetOutput]
  flipNO BypassOn

vtkImageFlip flipX
  flipX SetInput [imageFloat GetOutput]
  flipX SetFilteredAxes $VTK_IMAGE_X_AXIS 

vtkImageFlip flipY
  flipY SetInput [imageFloat GetOutput]
  flipY SetFilteredAxes $VTK_IMAGE_Y_AXIS 

vtkImageAppend append
  append AddInput [flipNO GetOutput]
  append AddInput [flipX GetOutput]
  append AddInput [flipY GetOutput]
  append SetAppendAxis $VTK_IMAGE_X_AXIS

#flip BypassOn
#flip PreserveImageExtentOn

vtkImageViewer viewer
viewer SetInput [append GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

#make interface
source WindowLevelInterface.tcl








