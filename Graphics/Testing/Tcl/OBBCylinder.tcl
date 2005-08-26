package require vtk
package require vtkinteraction

vtkCylinderSource cylinder
cylinder SetHeight 1
cylinder SetRadius 4
cylinder SetResolution 100
cylinder CappingOff

vtkTransform foo
foo RotateX 20
foo RotateY 10
foo RotateZ 27
foo Scale 1 .7 .3

vtkTransformPolyDataFilter transPD
transPD SetInputConnection [cylinder GetOutputPort]
transPD SetTransform foo

vtkPolyDataMapper dataMapper
  dataMapper SetInputConnection [transPD GetOutputPort]
vtkActor model
  model SetMapper dataMapper
  [model GetProperty] SetColor 1 0 0


vtkOBBTree obb
  obb SetMaxLevel 10
  obb SetNumberOfCellsPerBucket 5
  obb AutomaticOff

vtkSpatialRepresentationFilter boxes
  boxes SetInputConnection [transPD GetOutputPort]
  boxes SetSpatialRepresentation obb
vtkExtractEdges boxEdges
  boxEdges SetInput [ boxes GetOutput ]
vtkPolyDataMapper boxMapper
  boxMapper SetInputConnection [boxEdges GetOutputPort]
  boxMapper SetResolveCoincidentTopology 1
vtkActor boxActor
  boxActor SetMapper boxMapper
  [boxActor GetProperty] SetAmbient 1
  [boxActor GetProperty] SetDiffuse 0
  [boxActor GetProperty] SetRepresentationToWireframe


vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor model
ren1 AddActor boxActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.5

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

