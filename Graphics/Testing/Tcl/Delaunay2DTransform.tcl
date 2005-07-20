package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create some points on a sphere such that the data is not in the form
# of z = f(x,y)
#

vtkMath math1
vtkPoints points
vtkFloatArray vectors
vectors SetNumberOfComponents 3
for {set i 0} {$i<100} {incr i 1} {
    set theta [math1 Random 0.31415 2.8]
    set phi [math1 Random 0.31415 2.8]
    eval points InsertPoint $i [expr cos($theta)*sin($phi)] [expr sin($theta)*sin($phi)] [expr cos($phi)]
    eval vectors InsertTuple3 $i [expr cos($theta)*sin($phi)] [expr sin($theta)*sin($phi)] [expr cos($phi)]
}

vtkPolyData profile
    profile SetPoints points
    [profile GetPointData] SetVectors vectors

# build a transform that rotates this data into z = f(x,y)
#
vtkTransform transform
transform RotateX 90


# triangulate the data using the specified transform
#
vtkDelaunay2D del1
    del1 SetInput profile
    del1 SetTransform transform
    del1 BoundingTriangulationOff
    del1 SetTolerance 0.001
    del1 SetAlpha 0.0

    
vtkShrinkPolyData shrink
    shrink SetInputConnection [del1 GetOutputPort]

vtkPolyDataMapper map
    map SetInputConnection [shrink GetOutputPort]

vtkActor triangulation
    triangulation SetMapper map
    [triangulation GetProperty] SetColor 1 0 0
    [triangulation GetProperty] BackfaceCullingOn

# Add the actors to the renderer, set the background and size
#
ren1 AddActor triangulation
ren1 SetBackground 1 1 1
renWin SetSize 300 300
renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.5
$cam1 Azimuth 90
$cam1 Elevation 30
$cam1 Azimuth -60

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


