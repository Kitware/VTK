package require vtk


# Image pipeline

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 63 0 63 1 93
  reader SetDataSpacing 3.2 3.2 1.5
  reader SetDataOrigin -100.8 -100.8 -69
  reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  reader SetDataMask 0x7fff
  reader Update

vtkPoints p1
vtkPoints p2

p1 InsertNextPoint 0 0 0
p2 InsertNextPoint -60 10 20
p1 InsertNextPoint -100 -100 -50
p2 InsertNextPoint -100 -100 -50
p1 InsertNextPoint -100 -100 50
p2 InsertNextPoint -100 -100 50
p1 InsertNextPoint -100 100 -50
p2 InsertNextPoint -100 100 -50
p1 InsertNextPoint -100 100 50
p2 InsertNextPoint -100 100 50
p1 InsertNextPoint 100 -100 -50
p2 InsertNextPoint 100 -100 -50
p1 InsertNextPoint 100 -100 50
p2 InsertNextPoint 100 -100 50
p1 InsertNextPoint 100 100 -50
p2 InsertNextPoint 100 100 -50
p1 InsertNextPoint 100 100 50
p2 InsertNextPoint 100 100 50

vtkThinPlateSplineTransform transform
  transform SetSourceLandmarks p1
  transform SetTargetLandmarks p2
  transform SetBasisToR

vtkImageReslice reslice
  reslice SetInput [reader GetOutput]
  reslice SetResliceTransform transform
  reslice SetInterpolationModeToLinear
  reslice SetOutputSpacing 1 1 1

vtkImageCacheFilter cac
  cac SetInput [reslice GetOutput]
  cac SetCacheSize 1000

cac SetInput [reslice GetOutput]

vtkImageViewer viewer
  viewer SetInput [cac GetOutput]
  viewer SetZSlice 90
  viewer SetColorWindow 2000
  viewer SetColorLevel 1000
  viewer Render

