package require vtk
package require vtkinteraction

vtkBox box
  box SetXMin 0 2 4
  box SetXMax 2 4 6

vtkSampleFunction sample
  sample SetSampleDimensions 30 30 30
  sample SetImplicitFunction box
  sample SetModelBounds 0 1.5 1 5 2 8 
  sample ComputeNormalsOn

vtkContourFilter contours
  contours SetInputConnection [sample GetOutputPort]
  contours GenerateValues 5 -0.5 1.5

vtkPolyDataWriter w
w SetInputConnection [contours GetOutputPort]
w SetFileName "junk.vtk"
#w Write

vtkPolyDataMapper contMapper
  contMapper SetInputConnection [contours GetOutputPort]
  contMapper SetScalarRange -0.5 1.5

vtkActor contActor
  contActor SetMapper contMapper

# We'll put a simple outline around the data.
vtkOutlineFilter outline
  outline SetInputConnection [sample GetOutputPort]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outline GetOutputPort]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  eval [outlineActor GetProperty] SetColor 0 0 0

# The usual rendering stuff.
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 500 500
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetBackground 1 1 1
ren1 AddActor contActor
ren1 AddActor outlineActor

vtkCamera camera
camera SetClippingRange 6.31875 20.689
camera SetFocalPoint 0.75 3 5
camera SetPosition 9.07114 -4.10065 -1.38712
camera SetViewAngle 30
camera SetViewUp -0.580577 -0.802756 0.13606

ren1 SetActiveCamera camera

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .
