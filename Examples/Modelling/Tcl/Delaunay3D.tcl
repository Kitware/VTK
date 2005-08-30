# This example shows how to use Delaunay3D with alpha shapes.
#
package require vtk
package require vtkinteraction

# The points to be triangulated are generated randomly in the unit cube
# located at the origin. The points are then associated with a vtkPolyData.
#
vtkMath math
vtkPoints points
for {set i 0} {$i<25} {incr i 1} {
    eval points InsertPoint $i [math Random 0 1] [math Random 0 1] [math Random 0 1]
}

vtkPolyData profile
    profile SetPoints points

# Delaunay3D is used to triangulate the points. The Tolerance is the distance
# that nearly coincident points are merged together. (Delaunay does better if
# points are well spaced.) The alpha value is the radius of circumcircles,
# circumspheres. Any mesh entity whose circumcircle is smaller than this
# value is output.
#
vtkDelaunay3D del
    del SetInput profile
    del SetTolerance 0.01
    del SetAlpha 0.2
    del BoundingTriangulationOff
    
# Shrink the result to help see it better.
vtkShrinkFilter shrink
    shrink SetInputConnection [del GetOutputPort]
    shrink SetShrinkFactor 0.9

vtkDataSetMapper map
    map SetInputConnection [shrink GetOutputPort]

vtkActor triangulation
    triangulation SetMapper map
    [triangulation GetProperty] SetColor 1 0 0

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor triangulation
ren1 SetBackground 1 1 1
renWin SetSize 250 250
renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.5

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



