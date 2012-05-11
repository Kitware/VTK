package require vtk
package require vtkinteraction
package require vtktesting

# Create fake data
#
vtkSphereSource ss
vtkPolyDataMapper mapper
  mapper SetInput [ss GetOutput]
vtkActor actor
  actor SetMapper mapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

iren Initialize
renWin Render

vtkTextWidget widget
widget SetInteractor iren
[widget GetTextActor] SetInput "This is a test"
widget On


# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground .1 .2 .4

iren Initialize
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
widget AddObserver ActivateEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
