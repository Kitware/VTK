catch {load vtktcl}
# this is a tcl version of the Mace example
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create a sphere source and actor
#
vtkSphereSource sphere

vtkPolyDataMapper   sphereMapper
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
vtkPlanes planes
vtkExtractPolyDataGeometry extract
    extract SetInput [sphere GetOutput]
    extract SetImplicitFunction planes

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
#ren1 AddActor sphereActor2

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# This zoom is used to perform the clipping
renWin Render
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 4.0

planes SetFrustumPlanes $cam1
sphereMapper SetInput [extract GetOutput]
renWin Render

$cam1 Zoom 0.6
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


