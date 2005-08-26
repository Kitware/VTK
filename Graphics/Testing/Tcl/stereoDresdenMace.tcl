package require vtk

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn
    renWin SetWindowName "vtk - Mace"
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

renWin SetStereoTypeToDresden
renWin StereoRenderOn

# create a sphere source and actor
#
vtkSphereSource sphere

vtkPolyDataMapper   sphereMapper
    sphereMapper SetInputConnection [sphere GetOutputPort]
    sphereMapper GlobalImmediateModeRenderingOn
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

# create the spikes using a cone source and the sphere source
#
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInputConnection [sphere GetOutputPort]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25
vtkPolyDataMapper spikeMapper
    spikeMapper SetInputConnection [glyph GetOutputPort]
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
iren AddObserver UserEvent {wm deiconify .vtkInteract}

ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize

# default arguments added so that the protoype matches
# as required in Python when the test is translated.
proc TkCheckAbort { { a 0 } { b 0 } } {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver "AbortCheckEvent" TkCheckAbort

# prevent the tk window from showing up then start the event loop
wm withdraw .


vtkMatrix4x4 mat
spikeActor SetUserMatrix mat

renWin Render
