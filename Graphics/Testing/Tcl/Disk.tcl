package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkDiskSource disk
    disk SetInnerRadius 1.0
    disk SetOuterRadius 2.0
    disk SetRadialResolution 1
    disk SetCircumferentialResolution 20

vtkPolyDataMapper diskMapper
    diskMapper SetInput [disk GetOutput]
vtkActor diskActor
    diskActor SetMapper diskMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor diskActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 200 200

# Get handles to some useful objects
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
renWin Render


# prevent the tk window from showing up then start the event loop
wm withdraw .



