package require vtk
package require vtkinteraction
package require vtktesting

# Test the quadric decimation LOD actor
#

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# pipeline stuff
#
vtkSphereSource sphere
  sphere SetPhiResolution 150
  sphere SetThetaResolution 150

vtkPolyDataMapper mapper
  mapper SetInputConnection [sphere GetOutputPort]

vtkQuadricLODActor actor
  actor SetMapper mapper
eval [actor GetProperty] SetDiffuseColor $tomato
[actor GetProperty] SetDiffuse .8
[actor GetProperty] SetSpecular .4
[actor GetProperty] SetSpecularPower 30

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1

renWin SetSize 300 300
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .
