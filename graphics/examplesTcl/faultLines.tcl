catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Create a constrained Delaunay triangulation following fault lines.

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# get some nice colors
source $VTK_TCL/colors.tcl

# create some points
#
vtkPolyDataReader reader
  reader SetFileName "$VTK_DATA/faults.vtk"

# triangulate them
#
vtkCleanPolyData cleaner
    cleaner SetInput [reader GetOutput]
vtkDelaunay2D del
    del SetInput [cleaner GetOutput]
    del SetSource [cleaner GetOutput]
    del SetTolerance 0.00001
vtkPolyDataNormals normals
    normals SetInput [del GetOutput]
vtkPolyDataMapper mapMesh
    mapMesh SetInput [normals GetOutput]
vtkActor meshActor
    meshActor SetMapper mapMesh
    eval [meshActor GetProperty] SetColor $beige

vtkTubeFilter tuber
    tuber SetInput [cleaner GetOutput]
    tuber SetRadius 25
vtkPolyDataMapper mapLines
    mapLines SetInput [tuber GetOutput]
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
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
iren LightFollowCameraOff
iren Initialize

renWin SetFileName "faultLines.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


