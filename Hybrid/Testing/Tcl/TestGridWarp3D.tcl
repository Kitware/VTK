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

vtkTransformToGrid gridThinPlate
  gridThinPlate SetInput transform
  gridThinPlate SetGridExtent 0 64 0 64 0 50
  gridThinPlate SetGridSpacing 3.2 3.2 3.0
  gridThinPlate SetGridOrigin -102.4 -102.4 -75
  gridThinPlate SetGridScalarTypeToUnsignedChar

vtkGridTransform gridTransform
  gridTransform SetDisplacementGrid [gridThinPlate GetOutput]
  gridTransform SetDisplacementShift [gridThinPlate GetDisplacementShift]
  gridTransform SetDisplacementScale [gridThinPlate GetDisplacementScale]

vtkImageReslice reslice
  reslice SetInput [reader GetOutput]
  reslice SetResliceTransform gridTransform
  reslice SetInterpolationModeToLinear
  reslice SetOutputSpacing 1 1 1

vtkImageViewer viewer
  viewer SetInput [reslice GetOutput]
  viewer SetZSlice 70
  viewer SetColorWindow 2000
  viewer SetColorLevel 1000
  viewer Render



