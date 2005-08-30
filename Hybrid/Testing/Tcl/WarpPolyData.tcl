package require vtk

# this example tests the warping of PolyData using thin plate splines
# and with grid transforms using different interpolation modes

# create a rendering window
vtkRenderWindow renWin
renWin SetSize 600 300

vtkSphereSource sphere
  sphere SetThetaResolution 20
  sphere SetPhiResolution 20

vtkPolyDataNormals ap
  ap SetInputConnection [sphere GetOutputPort]   

#---------------------------
# thin plate spline transform
vtkPoints spoints
  spoints SetNumberOfPoints 10
  spoints SetPoint 0 0.000 0.000 0.500
  spoints SetPoint 1 0.000 0.000 -0.500
  spoints SetPoint 2 0.433 0.000 0.250
  spoints SetPoint 3 0.433 0.000 -0.250
  spoints SetPoint 4 -0.000 0.433 0.250
  spoints SetPoint 5 -0.000 0.433 -0.250
  spoints SetPoint 6 -0.433 -0.000 0.250
  spoints SetPoint 7 -0.433 -0.000 -0.250
  spoints SetPoint 8 0.000 -0.433 0.250
  spoints SetPoint 9 0.000 -0.433 -0.250

vtkPoints tpoints
  tpoints SetNumberOfPoints 10
  tpoints SetPoint 0 0.000 0.000 0.800
  tpoints SetPoint 1 0.000 0.000 -0.200
  tpoints SetPoint 2 0.433 0.000 0.350
  tpoints SetPoint 3 0.433 0.000 -0.150
  tpoints SetPoint 4 -0.000 0.233 0.350
  tpoints SetPoint 5 -0.000 0.433 -0.150
  tpoints SetPoint 6 -0.433 -0.000 0.350
  tpoints SetPoint 7 -0.433 -0.000 -0.150
  tpoints SetPoint 8 0.000 -0.233 0.350
  tpoints SetPoint 9 0.000 -0.433 -0.150

vtkThinPlateSplineTransform thin
  thin SetSourceLandmarks spoints
  thin SetTargetLandmarks tpoints
  thin SetBasisToR2LogR
#  thin Inverse

vtkGeneralTransform t1
  t1 SetInput thin

vtkTransformPolyDataFilter f11
f11 SetInputConnection [ap GetOutputPort]
f11 SetTransform t1

vtkDataSetMapper m11
m11 SetInputConnection [f11 GetOutputPort]

vtkActor a11
a11 SetMapper m11
a11 RotateY 90
[a11 GetProperty] SetColor 1 0 0
#[a11 GetProperty] SetRepresentationToWireframe

vtkRenderer ren11
ren11 SetViewport 0.0 0.5 0.25 1.0
ren11 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren11 AddActor a11
renWin AddRenderer ren11

# inverse thin plate spline transform
vtkTransformPolyDataFilter f12
f12 SetInputConnection [ap GetOutputPort]
f12 SetTransform [t1 GetInverse]

vtkDataSetMapper m12
m12 SetInputConnection [f12 GetOutputPort]

vtkActor a12
a12 SetMapper m12
a12 RotateY 90
[a12 GetProperty] SetColor 0.9 0.9 0
#[a12 GetProperty] SetRepresentationToWireframe

vtkRenderer ren12
ren12 SetViewport 0.0 0.0 0.25 0.5
ren12 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren12 AddActor a12
renWin AddRenderer ren12


#--------------------------
# grid transform, cubic interpolation
vtkTransformToGrid gridTrans
gridTrans SetInput t1
gridTrans SetGridOrigin -1.5 -1.5 -1.5
gridTrans SetGridExtent 0 60 0 60 0 60
gridTrans SetGridSpacing 0.05 0.05 0.05

vtkGridTransform t2
t2 SetDisplacementGrid [gridTrans GetOutput]
t2 SetInterpolationModeToCubic

vtkTransformPolyDataFilter f21
f21 SetInputConnection [ap GetOutputPort]
f21 SetTransform t2

vtkDataSetMapper m21
m21 SetInputConnection [f21 GetOutputPort]

vtkActor a21
a21 SetMapper m21
a21 RotateY 90
[a21 GetProperty] SetColor 1 0 0
#[a21 GetProperty] SetRepresentationToWireframe

vtkRenderer ren21
ren21 SetViewport 0.25 0.5 0.50 1.0
ren21 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren21 AddActor a21
renWin AddRenderer ren21

# inverse
vtkTransformPolyDataFilter f22
f22 SetInputConnection [ap GetOutputPort]
f22 SetTransform [t2 GetInverse]

vtkDataSetMapper m22
m22 SetInputConnection [f22 GetOutputPort]

vtkActor a22
a22 SetMapper m22
a22 RotateY 90
[a22 GetProperty] SetColor 0.9 0.9 0
#[a22 GetProperty] SetRepresentationToWireframe

vtkRenderer ren22
ren22 SetViewport 0.25 0.0 0.50 0.5
ren22 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren22 AddActor a22
renWin AddRenderer ren22


#--------------------------
# grid transform, linear
vtkGridTransform t3
t3 SetDisplacementGrid [gridTrans GetOutput]
t3 SetInterpolationModeToLinear

vtkTransformPolyDataFilter f31
f31 SetInputConnection [ap GetOutputPort]
f31 SetTransform t3

vtkDataSetMapper m31
m31 SetInputConnection [f31 GetOutputPort]

vtkActor a31
a31 SetMapper m31
a31 RotateY 90
[a31 GetProperty] SetColor 1 0 0
#[a31 GetProperty] SetRepresentationToWireframe

vtkRenderer ren31
ren31 SetViewport 0.50 0.5 0.75 1.0
ren31 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren31 AddActor a31
renWin AddRenderer ren31

# inverse 
vtkTransformPolyDataFilter f32
f32 SetInputConnection [ap GetOutputPort]
f32 SetTransform [t3 GetInverse]

vtkDataSetMapper m32
m32 SetInputConnection [f32 GetOutputPort]

vtkActor a32
a32 SetMapper m32
a32 RotateY 90
[a32 GetProperty] SetColor 0.9 0.9 0
#[a32 GetProperty] SetRepresentationToWireframe

vtkRenderer ren32
ren32 SetViewport 0.5 0.0 0.75 0.5
ren32 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren32 AddActor a32
renWin AddRenderer ren32


#--------------------------
# grid transform, nearest
vtkGridTransform t4
t4 SetDisplacementGrid [gridTrans GetOutput]
t4 SetInterpolationModeToNearestNeighbor
t4 SetInverseTolerance 0.05

vtkTransformPolyDataFilter f41
f41 SetInputConnection [ap GetOutputPort]
f41 SetTransform t4

vtkDataSetMapper m41
m41 SetInputConnection [f41 GetOutputPort]

vtkActor a41
a41 SetMapper m41
a41 RotateY 90
[a41 GetProperty] SetColor 1 0 0
#[a41 GetProperty] SetRepresentationToWireframe

vtkRenderer ren41
ren41 SetViewport 0.75 0.5 1.0 1.0
ren41 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren41 AddActor a41
renWin AddRenderer ren41

#inverse
vtkTransformPolyDataFilter f42
f42 SetInputConnection [ap GetOutputPort]
f42 SetTransform [t4 GetInverse]

vtkDataSetMapper m42
m42 SetInputConnection [f42 GetOutputPort]

vtkActor a42
a42 SetMapper m42
a42 RotateY 90
[a42 GetProperty] SetColor 0.9 0.9 0
#[a42 GetProperty] SetRepresentationToWireframe

vtkRenderer ren42
ren42 SetViewport 0.75 0.0 1.0 0.5
ren42 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren42 AddActor a42
renWin AddRenderer ren42

t1 RotateX -100
t1 PostMultiply
t1 RotateX +100

renWin Render







