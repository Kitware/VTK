catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# this is a tcl version of the Mace example
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# create a sphere source and actor
#
vtkSphereSource sphere
  sphere SetThetaResolution 36
  sphere SetPhiResolution 18
  sphere SetRadius 1.5

vtkPolyDataMapper  sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkActor sphereActor
    sphereActor SetMapper sphereMapper
vtkActor sphereActor2
    sphereActor2 SetMapper sphereMapper
    [sphereActor2 GetProperty] SetRepresentationToWireframe
    [sphereActor2 GetProperty] SetColor 0 0 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetWindowName "vtk - Mace"
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Extraction stuff
vtkCylinder cylfunc
  cylfunc SetRadius 0.5

vtkExtractPolyDataGeometry extract
    extract SetInput [sphere GetOutput]
    extract SetImplicitFunction cylfunc

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

sphereMapper SetInput [extract GetOutput]

vtkCamera cam1
  cam1 SetPosition 4 4 3

ren1 SetActiveCamera cam1

renWin Render


# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

iren Initialize
renWin SetFileName "extractPolyData.tcl.ppm"
#renWin SaveImageAsPPM

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .


