catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl

# First create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren   [$renWin MakeRenderWindowInteractor]

# create some points
#
vtkMath math
vtkFloatPoints points
for {set i 0} {$i<1000} {incr i 1} {
    eval points InsertPoint $i [math Random 0 1] [math Random 0 1] 0.0
}

vtkPolyData profile
    profile SetPoints points

# triangulate them
#
vtkDelaunay2D del
    del SetInput profile
    del BoundingTriangulationOn
    del SetTolerance 0.001
    del SetAlpha 0.0
    del Update
    
vtkShrinkPolyData shrink
    shrink SetInput [del GetOutput]

vtkPolyMapper map
    map SetInput [shrink GetOutput]

vtkActor triangulation
    triangulation SetMapper map
    [triangulation GetProperty] SetColor 1 0 0

# Add the actors to the renderer, set the background and size
#
$ren1 AddActor triangulation
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500
$renWin Render

set cam1 [$ren1 GetActiveCamera]
$cam1 Zoom 1.5

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

$renWin Render

#$renWin SetFileName Delaunay2D.tcl.ppm
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


