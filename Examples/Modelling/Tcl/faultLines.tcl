# Create a constrained Delaunay triangulation following fault lines. The
# fault lines serve as constraint edges in the Delaunay triangulation.
#
package require vtk
package require vtkinteraction
package require vtktesting

# Generate some points by reading a VTK data file. The data file also has
# edges that represent constraint lines. This is originally from a geologic
# horizon.
#
vtkPolyDataReader reader
  reader SetFileName "$VTK_DATA_ROOT/Data/faults.vtk"

# Perform a 2D triangulation with constraint edges.
#
vtkDelaunay2D del
    del SetInputConnection [reader GetOutputPort]
    del SetSourceConnection [reader GetOutputPort]
    del SetTolerance 0.00001
vtkPolyDataNormals normals
    normals SetInputConnection [del GetOutputPort]
vtkPolyDataMapper mapMesh
    mapMesh SetInputConnection [normals GetOutputPort]
vtkActor meshActor
    meshActor SetMapper mapMesh
    eval [meshActor GetProperty] SetColor $beige

# Now pretty up the mesh with tubed edges and balls at the vertices.
vtkTubeFilter tuber
    tuber SetInputConnection [reader GetOutputPort]
    tuber SetRadius 25
vtkPolyDataMapper mapLines
    mapLines SetInputConnection [tuber GetOutputPort]
vtkActor linesActor
    linesActor SetMapper mapLines
    [linesActor GetProperty] SetColor 1 0 0
    eval [linesActor GetProperty] SetColor $tomato

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor linesActor
ren1 AddActor meshActor
ren1 SetBackground 1 1 1
renWin SetSize 350 250

vtkCamera cam1
    cam1 SetClippingRange 2580 129041
    cam1 SetFocalPoint 461550 6.58e+006 2132
    cam1 SetPosition 463960 6.559e+06 16982
    cam1 SetViewUp -0.321899 0.522244 0.78971
vtkLight light
    light SetPosition 0 0 1
    eval light SetFocalPoint 0 0 0
ren1 SetActiveCamera cam1
ren1 AddLight light

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
iren LightFollowCameraOff
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
iren Start

