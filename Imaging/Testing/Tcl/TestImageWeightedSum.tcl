package require vtk

# Image pipeline

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 63 0 63 1 93
  reader SetFilePrefix "${VTK_DATA_ROOT}/Data/headsq/quarter"
  reader SetDataMask 0x7fff

vtkImageMagnify mag
  mag SetInput [reader GetOutput]
  mag SetMagnificationFactors 4 4 1

vtkImageThreshold th
  th SetInput [mag GetOutput]
  th SetReplaceIn 1
  th SetReplaceOut 1
  th ThresholdBetween -1000 1000
  th SetOutValue 0
  th SetInValue 2000

vtkImageCast cast
  cast SetInput [mag GetOutput]
  cast SetOutputScalarTypeToFloat

vtkImageCast cast2
  cast2 SetInput [th GetOutput]
  cast2 SetOutputScalarTypeToFloat

vtkImageWeightedSum sum
  sum AddInput [cast GetOutput]
  sum AddInput [cast2 GetOutput]
  sum SetWeight 0 10
  sum SetWeight 1 4

vtkImageViewer viewer
  viewer SetInput [sum GetOutput]
  viewer SetZSlice 22
  viewer SetColorWindow 1819
  viewer SetColorLevel 939

sum SetWeight 0 1

viewer Render

wm withdraw .
