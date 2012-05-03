package require vtk
package require vtkinteraction
package require vtktesting

#
# Demonstrate the use of clipping and capping on polyhedral data
#

# create a sphere and clip it
#
vtkSphereSource sphere
    sphere SetRadius 1
    sphere SetPhiResolution 10
    sphere SetThetaResolution 10

vtkPlane plane1
    plane1 SetOrigin 0.3 0.3 0.3
    plane1 SetNormal -1 -1 -1

vtkPlane plane2
    plane2 SetOrigin 0.5 0 0
    plane2 SetNormal -1 0 0

vtkPlaneCollection planes
    planes AddItem plane1
    planes AddItem plane2

# stripper just increases coverage
vtkStripper stripper
    stripper SetInputConnection [sphere GetOutputPort]

vtkClipClosedSurface clipper
    clipper SetInputConnection [stripper GetOutputPort]
    clipper SetClippingPlanes planes

vtkClipClosedSurface clipperOutline
    clipperOutline SetInputConnection [stripper GetOutputPort]
    clipperOutline SetClippingPlanes planes
    clipperOutline GenerateFacesOff
    clipperOutline GenerateOutlineOn

vtkPolyDataMapper sphereMapper
    sphereMapper SetInputConnection [clipper GetOutputPort]

vtkPolyDataMapper clipperOutlineMapper
    clipperOutlineMapper SetInputConnection [clipperOutline GetOutputPort]

vtkActor clipActor
    clipActor SetMapper sphereMapper
    [clipActor GetProperty] SetColor 0.8 0.05 0.2

vtkActor clipOutlineActor
    clipOutlineActor SetMapper clipperOutlineMapper
    [clipOutlineActor GetProperty] SetColor 0 1 0
    clipOutlineActor SetPosition 0.001 0.001 0.001

# create an outline

vtkOutlineFilter outline
  outline SetInputConnection [sphere GetOutputPort]
  outline GenerateFacesOn

vtkClipClosedSurface outlineClip
  outlineClip SetClippingPlanes planes
  outlineClip SetInputConnection [outline GetOutputPort]
  outlineClip GenerateFacesOff
  outlineClip GenerateOutlineOn
  outlineClip SetScalarModeToColors
  outlineClip SetClipColor 0 1 0

vtkDataSetMapper outlineMapper
  outlineMapper SetInputConnection [outlineClip GetOutputPort]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  outlineActor SetPosition 0.001 0.001 0.001

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetMultiSamples 0
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor clipActor
ren1 AddActor clipOutlineActor
ren1 AddActor outlineActor
ren1 SetBackground 1 1 1
ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

renWin SetSize 300 300
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
