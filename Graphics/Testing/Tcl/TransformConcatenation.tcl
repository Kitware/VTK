package require vtk

# This example demonstrates how to set up flexible joints using 
# the transformation pipeline and vtkTransformPolyDataFilter.


# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1

# set up first set of polydata
vtkCylinderSource c1
c1 SetHeight 1.6
c1 SetRadius 0.2
c1 SetCenter 0 0.8 0

vtkTransform t1

vtkTransformPolyDataFilter f1
f1 SetInput [c1 GetOutput]
f1 SetTransform t1

vtkDataSetMapper m1
m1 SetInput [f1 GetOutput]

vtkActor a1
a1 SetMapper m1
[a1 GetProperty] SetColor 1 0 0


# set up second set, at a relative transform to the first
vtkCylinderSource c2
c2 SetHeight 1.6
c2 SetRadius 0.15
c2 SetCenter 0 0.8 0

# relative rotation for first joint
vtkTransform joint1

# set input to initial transform
vtkTransform t2
t2 SetInput t1
t2 Translate 0 1.6 0
t2 Concatenate joint1 

vtkTransformPolyDataFilter f2
f2 SetInput [c2 GetOutput]
f2 SetTransform t2

vtkDataSetMapper m2
m2 SetInput [f2 GetOutput]

vtkActor a2
a2 SetMapper m2
[a2 GetProperty] SetColor 0.0 0.7 1.0


# set up third set, at a relative transform to the second
vtkCylinderSource c3
c3 SetHeight 0.5
c3 SetRadius 0.1
c3 SetCenter 0 0.25 0

# relative rotation
vtkTransform joint2

# set input to previous transform
vtkTransform t3
t3 SetInput t2
t3 Translate 0 1.6 0
t3 Concatenate joint2

vtkTransformPolyDataFilter f3
f3 SetInput [c3 GetOutput]
f3 SetTransform t3

vtkDataSetMapper m3
m3 SetInput [f3 GetOutput]

vtkActor a3
a3 SetMapper m3
[a3 GetProperty] SetColor 0.9 0.9 0


# add actors to renderer
ren1 AddActor a1
ren1 AddActor a2
ren1 AddActor a3

# set clipping range
ren1 ResetCamera -1 1 -0.1 2 -3 3

# set angles for first joint
set phi2   70
set theta2 85

# set angles for second joint
set phi3   50
set theta3 90

joint1 Identity
joint1 RotateY $phi2
joint1 RotateX $theta2

joint2 Identity
joint2 RotateY $phi3
joint2 RotateX $theta3

renWin Render




