package require vtktcl

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
  sphere SetThetaResolution 6
  sphere SetRadius 1

vtkElevationFilter el
  el SetInput [sphere GetOutput]
  el SetLowPoint 0 0 0
  el SetHighPoint 0 0 1

vtkQuadricDecimation mesh
  mesh SetInput [el GetOutput]
  mesh SetMaximumCost 6
  mesh SetMaximumCollapsedEdges 1

vtkPolyDataMapper mapper
  mapper SetInput [mesh GetOutput]

vtkActor actor
  actor SetMapper mapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 400 400
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .


renWin Render
