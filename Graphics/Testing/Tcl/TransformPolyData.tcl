package require vtk

# create a rendering window
vtkRenderWindow renWin
renWin SetSize 600 300

# set up first set of polydata
vtkPlaneSource p1
p1 SetOrigin  0.5  0.508 -0.5
p1 SetPoint1 -0.5  0.508 -0.5
p1 SetPoint2  0.5  0.508  0.5
p1 SetXResolution 5
p1 SetYResolution 5

vtkPlaneSource p2
p2 SetOrigin -0.508  0.5 -0.5
p2 SetPoint1 -0.508 -0.5 -0.5
p2 SetPoint2 -0.508  0.5  0.5
p2 SetXResolution 5
p2 SetYResolution 5

vtkPlaneSource p3
p3 SetOrigin -0.5 -0.508 -0.5
p3 SetPoint1  0.5 -0.508 -0.5
p3 SetPoint2 -0.5 -0.508  0.5
p3 SetXResolution 5
p3 SetYResolution 5

vtkPlaneSource p4
p4 SetOrigin  0.508 -0.5 -0.5
p4 SetPoint1  0.508  0.5 -0.5
p4 SetPoint2  0.508 -0.5  0.5
p4 SetXResolution 5
p4 SetYResolution 5

vtkPlaneSource p5
p5 SetOrigin  0.5  0.5 -0.508
p5 SetPoint1  0.5 -0.5 -0.508
p5 SetPoint2 -0.5  0.5 -0.508
p5 SetXResolution 5
p5 SetYResolution 5

vtkPlaneSource p6
p6 SetOrigin  0.5  0.5  0.508
p6 SetPoint1 -0.5  0.5  0.508
p6 SetPoint2  0.5 -0.5  0.508
p6 SetXResolution 5
p6 SetYResolution 5

# append together
vtkAppendPolyData ap
ap AddInput [p1 GetOutput]
ap AddInput [p2 GetOutput]
ap AddInput [p3 GetOutput]
ap AddInput [p4 GetOutput]
ap AddInput [p5 GetOutput]
ap AddInput [p6 GetOutput]

#--------------------------
# identity transform
vtkIdentityTransform t1

vtkTransformPolyDataFilter f11
f11 SetInput [ap GetOutput]
f11 SetTransform t1

vtkDataSetMapper m11
m11 SetInput [f11 GetOutput]

vtkActor a11
a11 SetMapper m11
[a11 GetProperty] SetColor 1 0 0
[a11 GetProperty] SetRepresentationToWireframe

vtkRenderer ren11
ren11 SetViewport 0.0 0.5 0.25 1.0
ren11 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren11 AddActor a11
renWin AddRenderer ren11

# inverse identity transform
vtkTransformPolyDataFilter f12
f12 SetInput [ap GetOutput]
f12 SetTransform [t1 GetInverse]

vtkDataSetMapper m12
m12 SetInput [f12 GetOutput]

vtkActor a12
a12 SetMapper m12
[a12 GetProperty] SetColor 0.9 0.9 0
[a12 GetProperty] SetRepresentationToWireframe

vtkRenderer ren12
ren12 SetViewport 0.0 0.0 0.25 0.5
ren12 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren12 AddActor a12
renWin AddRenderer ren12


#--------------------------
# linear transform
vtkTransform t2
t2 RotateX 50
t2 RotateY 30
t2 Translate 0.2 0.1 -0.15

vtkTransformPolyDataFilter f21
f21 SetInput [ap GetOutput]
f21 SetTransform t2

vtkDataSetMapper m21
m21 SetInput [f21 GetOutput]

vtkActor a21
a21 SetMapper m21
[a21 GetProperty] SetColor 1 0 0
[a21 GetProperty] SetRepresentationToWireframe

vtkRenderer ren21
ren21 SetViewport 0.25 0.5 0.50 1.0
ren21 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren21 AddActor a21
renWin AddRenderer ren21

# inverse linear transform
vtkTransformPolyDataFilter f22
f22 SetInput [ap GetOutput]
f22 SetTransform [t2 GetInverse]

vtkDataSetMapper m22
m22 SetInput [f22 GetOutput]

vtkActor a22
a22 SetMapper m22
[a22 GetProperty] SetColor 0.9 0.9 0
[a22 GetProperty] SetRepresentationToWireframe

vtkRenderer ren22
ren22 SetViewport 0.25 0.0 0.50 0.5
ren22 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren22 AddActor a22
renWin AddRenderer ren22


#--------------------------
# perspective transform
vtkMatrix4x4 matrix
matrix SetElement 3 0 0.1
matrix SetElement 3 1 0.2
matrix SetElement 3 2 0.5

vtkPerspectiveTransform t3
t3 SetMatrix matrix

vtkTransformPolyDataFilter f31
f31 SetInput [ap GetOutput]
f31 SetTransform t3

vtkDataSetMapper m31
m31 SetInput [f31 GetOutput]

vtkActor a31
a31 SetMapper m31
[a31 GetProperty] SetColor 1 0 0
[a31 GetProperty] SetRepresentationToWireframe

vtkRenderer ren31
ren31 SetViewport 0.50 0.5 0.75 1.0
ren31 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren31 AddActor a31
renWin AddRenderer ren31

# inverse linear transform
vtkTransformPolyDataFilter f32
f32 SetInput [ap GetOutput]
f32 SetTransform [t3 GetInverse]

vtkDataSetMapper m32
m32 SetInput [f32 GetOutput]

vtkActor a32
a32 SetMapper m32
[a32 GetProperty] SetColor 0.9 0.9 0
[a32 GetProperty] SetRepresentationToWireframe

vtkRenderer ren32
ren32 SetViewport 0.5 0.0 0.75 0.5
ren32 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren32 AddActor a32
renWin AddRenderer ren32


#--------------------------
# perspective transform concatenation
vtkPerspectiveTransform t4
t4 Concatenate t1
t4 Concatenate t2
t4 Concatenate t3

vtkTransformPolyDataFilter f41
f41 SetInput [ap GetOutput]
f41 SetTransform t4

vtkDataSetMapper m41
m41 SetInput [f41 GetOutput]

vtkActor a41
a41 SetMapper m41
[a41 GetProperty] SetColor 1 0 0
[a41 GetProperty] SetRepresentationToWireframe

vtkRenderer ren41
ren41 SetViewport 0.75 0.5 1.0 1.0
ren41 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren41 AddActor a41
renWin AddRenderer ren41

# inverse linear transform
vtkTransformPolyDataFilter f42
f42 SetInput [ap GetOutput]
f42 SetTransform [t4 GetInverse]

vtkDataSetMapper m42
m42 SetInput [f42 GetOutput]

vtkActor a42
a42 SetMapper m42
[a42 GetProperty] SetColor 0.9 0.9 0
[a42 GetProperty] SetRepresentationToWireframe

vtkRenderer ren42
ren42 SetViewport 0.75 0.0 1.0 0.5
ren42 ResetCamera -0.5 0.5 -0.5 0.5 -1 1
ren42 AddActor a42
renWin AddRenderer ren42


renWin Render



