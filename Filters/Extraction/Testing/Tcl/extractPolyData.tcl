package require vtk
package require vtkinteraction

# Demonstrate how to extract polygonal cells with an implicit function
# get the interactor ui

# create a sphere source and actor
#
vtkSphereSource sphere
  sphere SetThetaResolution 8
  sphere SetPhiResolution 16
  sphere SetRadius 1.5

# Extraction stuff
vtkTransform t
    t RotateX 90
vtkCylinder cylfunc
    cylfunc SetRadius 0.5
    cylfunc SetTransform t
vtkExtractPolyDataGeometry extract
    extract SetInputConnection [sphere GetOutputPort]
    extract SetImplicitFunction cylfunc
    extract ExtractBoundaryCellsOn

vtkPolyDataMapper  sphereMapper
    sphereMapper SetInputConnection [extract GetOutputPort]
    sphereMapper GlobalImmediateModeRenderingOn

vtkActor sphereActor
    sphereActor SetMapper sphereMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetWindowName "vtk - extractPolyData"
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor

ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 30

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


