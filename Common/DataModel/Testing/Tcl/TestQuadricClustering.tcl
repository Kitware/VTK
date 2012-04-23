package require vtk
package require vtkinteraction
package require vtktesting

# Generate implicit model of a sphere
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

vtkPoints pts
  pts InsertNextPoint 0 0 0
  pts InsertNextPoint 1 0 0
  pts InsertNextPoint 0 1 0
  pts InsertNextPoint 0 0 1

vtkCellArray tris
  tris InsertNextCell 3
  tris InsertCellPoint 0
  tris InsertCellPoint 1
  tris InsertCellPoint 2
  tris InsertNextCell 3
  tris InsertCellPoint 0
  tris InsertCellPoint 2
  tris InsertCellPoint 3
  tris InsertNextCell 3
  tris InsertCellPoint 0
  tris InsertCellPoint 3
  tris InsertCellPoint 1
  tris InsertNextCell 3
  tris InsertCellPoint 1
  tris InsertCellPoint 2
  tris InsertCellPoint 3

vtkPolyData polys
  polys SetPoints pts
  polys SetPolys tris

vtkQuadricClustering mesh
  mesh SetInputConnection [sphere GetOutputPort]
  mesh SetNumberOfXDivisions 10
  mesh SetNumberOfYDivisions 10
  mesh SetNumberOfZDivisions 10

vtkPolyDataMapper mapper
  mapper SetInputConnection [mesh GetOutputPort]
vtkActor actor
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
