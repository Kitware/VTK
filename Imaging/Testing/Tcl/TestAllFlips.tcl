package require vtktcl


# Image pipeline
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageCast imageFloat
  imageFloat SetInput [reader GetOutput]
  imageFloat SetOutputScalarTypeToFloat

vtkImageFlip flipX
  flipX SetInput [imageFloat GetOutput]
  flipX SetFilteredAxes 0

vtkImageFlip flipY
  flipY SetInput [imageFloat GetOutput]
  flipY SetFilteredAxes 1

vtkImageAppend imageAppend
  imageAppend AddInput [imageFloat GetOutput]
  imageAppend AddInput [flipX GetOutput]
  imageAppend AddInput [flipY GetOutput]
  imageAppend SetAppendAxis 0

#flip BypassOn
#flip PreserveImageExtentOn

vtkImageViewer viewer
viewer SetInput [imageAppend GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

#make interface
viewer Render








