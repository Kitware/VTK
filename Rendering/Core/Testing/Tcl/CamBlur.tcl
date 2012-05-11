package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create the piplinee, ball and spikes

vtkSphereSource sphere
  sphere SetThetaResolution 7 ; sphere SetPhiResolution 7
vtkPolyDataMapper sphereMapper
  sphereMapper SetInputConnection [sphere GetOutputPort]
vtkActor sphereActor
  sphereActor SetMapper sphereMapper
vtkActor sphereActor2
  sphereActor2 SetMapper sphereMapper
vtkConeSource cone
  cone SetResolution 5

vtkGlyph3D glyph
  glyph SetInputConnection [sphere GetOutputPort]
  glyph SetSourceConnection [cone GetOutputPort]
  glyph SetVectorModeToUseNormal
  glyph SetScaleModeToScaleByVector
  glyph SetScaleFactor 0.25

vtkPolyDataMapper spikeMapper
  spikeMapper SetInputConnection [glyph GetOutputPort]
vtkActor spikeActor
  spikeActor SetMapper spikeMapper
vtkActor spikeActor2
  spikeActor2 SetMapper spikeMapper

# set the actors position and scale
spikeActor SetPosition 0 0.7 0
sphereActor SetPosition 0 0.7 0
spikeActor2 SetPosition 0 -1 -10
sphereActor2 SetPosition 0 -1 -10
spikeActor2 SetScale 1.5 1.5 1.5
sphereActor2 SetScale 1.5 1.5 1.5


ren1 AddActor sphereActor
ren1 AddActor spikeActor
ren1 AddActor sphereActor2
ren1 AddActor spikeActor2
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 200 200
renWin DoubleBufferOff

# do the first render and then zoom in a little
renWin Render
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
[ren1 GetActiveCamera] Zoom 1.8
[ren1 GetActiveCamera] SetFocalDisk 0.05

renWin SetFDFrames 11
renWin Render;

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName CamBlur.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

