package require vtk
package require vtkinteraction
package require vtktesting

# Common script code used to create sphere/glyph (spike)

vtkRenderer ren1

vtkRenderWindow renWin
    renWin AddRenderer ren1

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
    iren SetDesiredUpdateRate .00001
    iren AddObserver UserEvent {wm deiconify .vtkInteract}

# Create a sphere source and actor

vtkSphereSource sphere

vtkPolyDataMapper sphereMapper
    sphereMapper SetInputConnection [sphere GetOutputPort]

vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

eval [sphereActor GetProperty] SetDiffuseColor $banana
eval [sphereActor GetProperty] SetSpecular .4
eval [sphereActor GetProperty] SetSpecularPower 20

# Create the spikes using a cone source and the sphere source

vtkConeSource cone
  cone SetResolution 20

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

eval [spikeActor GetProperty] SetDiffuseColor $tomato
eval [spikeActor GetProperty] SetSpecular .4
eval [spikeActor GetProperty] SetSpecularPower 20

# Add the actors to the renderer, set the background and size

ren1 AddActor sphereActor
ren1 AddActor spikeActor
ren1 SetBackground 0.1 0.2 0.4

renWin SetSize 300 300

# Render the image

ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
$cam1 Azimuth 30
$cam1 Elevation 30

renWin Render

# Prevent the tk window from showing up then start the event loop

wm withdraw .
