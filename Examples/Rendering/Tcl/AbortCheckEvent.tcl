package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# Create a sphere source and actor
vtkSphereSource sphere
  sphere SetThetaResolution 40
  sphere SetPhiResolution 40
vtkPolyDataMapper sphereMapper
  sphereMapper SetInputConnection [sphere GetOutputPort]
  sphereMapper GlobalImmediateModeRenderingOn
vtkLODActor sphereActor
  sphereActor SetMapper sphereMapper

# Create the spikes using a cone source and the sphere source
vtkConeSource cone
vtkGlyph3D glyph
  glyph SetInputConnection [sphere GetOutputPort]
  glyph SetSourceConnection [cone GetOutputPort]
  glyph SetVectorModeToUseNormal
  glyph SetScaleModeToScaleByVector
  glyph SetScaleFactor 0.25
vtkPolyDataMapper spikeMapper
  spikeMapper SetInputConnection [glyph GetOutputPort]
vtkLODActor spikeActor
  spikeActor SetMapper spikeMapper

# Add the actors to the renderer, set the background and size renWin
ren1 AddActor sphereActor
ren1 AddActor spikeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# Render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize

proc TkCheckAbort {} {
  if {[renWin GetEventPending] != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent TkCheckAbort

# Prevent the tk window from appearing; start the event loop
wm withdraw .
