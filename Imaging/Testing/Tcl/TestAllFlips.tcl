package require vtk


# Image pipeline
vtkImageReader reader
[reader GetExecutive] SetReleaseDataFlag 0 0
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageCast imageFloat
  imageFloat SetInputConnection [reader GetOutputPort]
  imageFloat SetOutputScalarTypeToFloat

vtkImageFlip flipX
  flipX SetInputConnection [imageFloat GetOutputPort]
  flipX SetFilteredAxis 0

vtkImageFlip flipY
  flipY SetInputConnection [imageFloat GetOutputPort]
  flipY SetFilteredAxis 1
  flipY FlipAboutOriginOn

vtkImageAppend imageAppend
  imageAppend AddInputConnection [imageFloat GetOutputPort]
  imageAppend AddInputConnection [flipX GetOutputPort]
  imageAppend AddInputConnection [flipY GetOutputPort]
  imageAppend SetAppendAxis 0

vtkImageViewer viewer
viewer SetInputConnection [imageAppend GetOutputPort]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

#make interface
viewer Render








