package require vtk

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create some points
#
vtkMath math
vtkPoints points
for {set i 0} {$i<100} {incr i 1} {
    eval points InsertPoint $i [math Random 0 1] [math Random 0 1] 0.0
}

vtkPolyData profile
    profile SetPoints points

# triangulate them
#
vtkDelaunay2D del1
    del1 SetInputData profile
    del1 SetTolerance 0.001
    del1 SetAlpha 0.1


vtkShrinkPolyData shrink
    shrink SetInputConnection [del1 GetOutputPort]

vtkPolyDataMapper map
    map SetInputConnection [shrink GetOutputPort]

vtkActor triangulation
    triangulation SetMapper map
    [triangulation GetProperty] SetColor 1 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor triangulation
ren1 SetBackground 1 1 1
renWin SetSize 300 300
renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.5

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


