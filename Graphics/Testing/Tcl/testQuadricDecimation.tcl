package require vtk
package require vtkinteraction

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# pipeline stuff
#

vtkSphereSource sphere
  sphere SetPhiResolution 5
  sphere SetThetaResolution 5
  sphere SetRadius 1

vtkTransform xform
  xform Scale 1 2 3
  xform RotateWXYZ 32 3 1 2
  xform Scale 2 5 4

vtkTransformFilter xformFilter
  xformFilter SetInput [sphere GetOutput]
  xformFilter SetTransform xform

vtkElevationFilter el
  el SetInput [xformFilter GetOutput]
  el SetLowPoint 0 0 0
  el SetHighPoint 0 0 1

vtkQuadricDecimation mesh
  mesh SetInput [el GetOutput]
  mesh SetMaximumCost 850
  mesh SetMaximumCollapsedEdges 5

vtkPolyDataMapper mapper
  mapper SetInput [mesh GetOutput]

vtkActor actor
  actor SetMapper mapper

# Set up the camera parameters
#
vtkCamera camera
  camera SetPosition -58.3274 -.948032 17.7715
  camera SetFocalPoint -.172376 .389125 -.100301
  camera SetViewUp -.0374429 .998186 -.0471559
  camera SetViewAngle 30
  camera SetClippingRange 48.8511 76.1099

ren1 SetActiveCamera camera

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 400 400
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .


renWin Render
