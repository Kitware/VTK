package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create room profile
#
vtkPoints points
    points InsertPoint 0 0 0 0
    points InsertPoint 1 1 0 0
    points InsertPoint 2 1 1 0
    points InsertPoint 3 2 1 0

vtkCellArray lines
    lines InsertNextCell 4;#number of points
    lines InsertCellPoint 0
    lines InsertCellPoint 1
    lines InsertCellPoint 2
    lines InsertCellPoint 3

vtkPolyData profile
    profile SetPoints points
    profile SetLines lines

vtkTransform xfm
    xfm Translate 0 0 8
    xfm RotateZ 90

vtkTransformPolyDataFilter xfmPd
    xfmPd SetInput profile
    xfmPd SetTransform xfm

vtkAppendPolyData appendPD
  appendPD AddInput profile
  appendPD AddInput [xfmPd GetOutput]

# extrude profile to make wall
#
vtkRuledSurfaceFilter extrude
    extrude SetInput [appendPD GetOutput]
    extrude SetResolution 51 51
    extrude SetRuledModeToResample

vtkPolyDataMapper map
    map SetInput [extrude GetOutput]

vtkActor wall
    wall SetMapper map
    [wall GetProperty] SetColor 0.3800 0.7000 0.1600


# Add the actors to the renderer, set the background and size
#
ren1 AddActor wall
ren1 SetBackground 1 1 1

renWin SetSize 200 200

[ren1 GetActiveCamera] SetPosition 12.9841 -1.81551 8.82706 
[ren1 GetActiveCamera] SetFocalPoint 0.5 1 4 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0.128644 -0.675064 -0.726456 
[ren1 GetActiveCamera] SetClippingRange 7.59758 21.3643 

renWin Render
# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

