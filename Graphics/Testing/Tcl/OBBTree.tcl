package require vtktcl

vtkSTLReader reader
  reader SetFileName $VTK_DATA_ROOT/Data/42400-IDGH.stl
vtkPolyDataMapper dataMapper
  dataMapper SetInput [reader GetOutput]
vtkActor model
  model SetMapper dataMapper
  [model GetProperty] SetColor 1 0 0

vtkOBBTree obb
  obb SetMaxLevel 4
  obb SetNumberOfCellsPerBucket 4
vtkSpatialRepresentationFilter boxes
  boxes SetInput [reader GetOutput]
  boxes SetSpatialRepresentation obb
vtkPolyDataMapper boxMapper
  boxMapper SetInput [boxes GetOutput]
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
renWin SetSize 400 400
[ren1 GetActiveCamera] Zoom 1.5

# render the image
#
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

