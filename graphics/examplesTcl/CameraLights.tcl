catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
# this is a tcl version of the Mace example
# get the interactor ui

source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetWindowName "vtk - CameraLights"
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# disable old light-tracking behavior
iren LightFollowCameraOff

# create lights
#
# CameraLights are defined in a local coordinate system where
# the camera is at (0, 0, 1), looking at (0, 0, 0), with up
# being (0, 1, 0)
#
vtkLight keyLight
  keyLight SetLightTypeToCameraLight
  keyLight SetPosition 0.25 1 0.75
  keyLight SetColor 1 1 0.9
ren1 AddLight keyLight

vtkLight fillLight
  fillLight SetLightTypeToCameraLight
  fillLight SetPosition -0.25 -1 0.75
  fillLight SetIntensity 0.2
  fillLight SetColor 0.75 0.75 1
ren1 AddLight fillLight

# A Headlight is always at the camera.
#
vtkLight headlight
  headlight SetLightTypeToHeadlight
  headlight SetIntensity 0.1
ren1 AddLight headlight

# create a sphere source and actor
#
vtkSphereSource sphere

vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

# create the spikes using a cone source and the sphere source
#
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25
vtkPolyDataMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkLODActor spikeActor
    spikeActor SetMapper spikeMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 AddActor spikeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize
#renWin SetFileName "mace.tcl.ppm"
#renWin SaveImageAsPPM

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .


