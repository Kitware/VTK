catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
# Test the wire generation of the sphere source
source $VTK_TCL/vtkInt.tcl

# create a sphere source and actor
#
vtkSphereSource sphere
    sphere LatLongTessellationOn
vtkExtractEdges edges
    edges SetInput [sphere GetOutput]
vtkTubeFilter tuber
    tuber SetInput [edges GetOutput]
    tuber SetRadius 0.01
    tuber SetNumberOfSides 6
vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [tuber GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetWindowName "vtk - Mace"
vtkInteractorStyleUnicam uni
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
#    iren SetInteractorStyle uni

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


