catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Try the sphere interactor.

source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetWindowName "vtk - Mace"
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a sphere source and actor
#
vtkSphereSource sphere

vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkActor sphereActor
    [sphereActor GetProperty] SetRepresentationToWireframe
    sphereActor SetMapper sphereMapper


# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 400

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize

vtkInteractorStyleSphere sphereStyle
eval sphereStyle SetCenter [sphere GetCenter]
sphereStyle SetRadius [sphere GetRadius]




vtkInteractorStyle defaultStyle
iren SetInteractorStyle sphereStyle

proc ToggleStyle {} {
    if {[string equal [iren GetInteractorStyle] defaultStyle]} {
	iren SetInteractorStyle sphereStyle
    } else {
	iren SetInteractorStyle defaultStyle
    }
}


proc ChangeProc {} {
    sphere SetCenter [sphereStyle GetCenterX] \
	             [sphereStyle GetCenterY] [sphereStyle GetCenterZ]
    sphere SetRadius [sphereStyle GetRadius]
}

sphereStyle SetChangeMethod {ChangeProc}




proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .




