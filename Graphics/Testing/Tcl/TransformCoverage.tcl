package require vtk

# this test covers a lot of the code in vtkAbstractTransform that
# is not covered elsewhere

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
vtkTransform tLinear
vtkPerspectiveTransform tPerspective
vtkGeneralTransform tGeneral

# set up a linear transformation
tLinear Scale 1.2 1.0 0.8
tLinear RotateX 30
tLinear RotateY 10
tLinear RotateZ 80
tLinear Translate 0.2 0.3 -0.1

# set up a perspective transform
tPerspective SetInput tLinear
tPerspective SetInput [tLinear GetInverse]
tPerspective Scale 2 2 2
# these should cancel
tPerspective AdjustViewport -0.5 0.5 -0.5 0.5 -1 1 -1 1
tPerspective AdjustViewport -1 1 -1 1 -0.5 0.5 -0.5 0.5 
# test shear transformation 
tPerspective Shear 0.2 0.3 0.0

# the following 6 operations cancel out
tPerspective RotateWXYZ 30 1 1 1
tPerspective RotateWXYZ -30 1 1 1
tPerspective Scale 2 2 2
tPerspective Scale 0.5 0.5 0.5
tPerspective Translate 10 0.1 0.3
tPerspective Translate -10 -0.1 -0.3

tPerspective Concatenate tLinear
# test push and pop
tPerspective Push
tPerspective RotateX 30
tPerspective RotateY 10
tPerspective RotateZ 80
tPerspective Translate 0.1 -0.2 0.0

# test copy of transforms
set tNew [tPerspective MakeTransform]
$tNew DeepCopy tPerspective

tPerspective Pop

# test general transform
tGeneral SetInput tLinear
tGeneral SetInput tPerspective
tGeneral PostMultiply
tGeneral Concatenate $tNew
tGeneral Concatenate [$tNew GetInverse]
tGeneral PreMultiply

# the following 6 operations cancel out
tGeneral RotateWXYZ 30 1 1 1
tGeneral RotateWXYZ -30 1 1 1
tGeneral Scale 2 2 2
tGeneral Scale 0.5 0.5 0.5
tGeneral Translate 10 0.1 0.3
tGeneral Translate -10 -0.1 -0.3

#--------------------------
# identity transform

vtkTransformPolyDataFilter f11
f11 SetInputConnection [ap GetOutputPort]
f11 SetTransform tLinear

vtkDataSetMapper m11
m11 SetInputConnection [f11 GetOutputPort]

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
f12 SetInputConnection [ap GetOutputPort]
f12 SetTransform [tLinear GetInverse]

vtkDataSetMapper m12
m12 SetInputConnection [f12 GetOutputPort]

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
vtkTransformPolyDataFilter f21
f21 SetInputConnection [ap GetOutputPort]
f21 SetTransform tPerspective

vtkDataSetMapper m21
m21 SetInputConnection [f21 GetOutputPort]

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
f22 SetInputConnection [ap GetOutputPort]
f22 SetTransform [tPerspective GetInverse]

vtkDataSetMapper m22
m22 SetInputConnection [f22 GetOutputPort]

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

vtkTransformPolyDataFilter f31
f31 SetInputConnection [ap GetOutputPort]
f31 SetTransform $tNew

vtkDataSetMapper m31
m31 SetInputConnection [f31 GetOutputPort]

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
f32 SetInputConnection [ap GetOutputPort]
f32 SetTransform [$tNew GetInverse]

vtkDataSetMapper m32
m32 SetInputConnection [f32 GetOutputPort]

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
vtkTransformPolyDataFilter f41
f41 SetInputConnection [ap GetOutputPort]
f41 SetTransform tGeneral

vtkDataSetMapper m41
m41 SetInputConnection [f41 GetOutputPort]

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
f42 SetInputConnection [ap GetOutputPort]
f42 SetTransform [tGeneral GetInverse]

vtkDataSetMapper m42
m42 SetInputConnection [f42 GetOutputPort]

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

# free what we did a MakeTransform on
$tNew UnRegister {}


