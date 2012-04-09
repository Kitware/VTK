package require vtk
package require vtkinteraction
package require vtktesting

# Manually create cells of various types: vertex, polyvertex, line,
# polyline, triangle, quad, pentagon, and triangle strip.
vtkPoints pts
  pts InsertPoint 0  0 0 0
  pts InsertPoint 1  0 1 0
  pts InsertPoint 2  0 2 0
  pts InsertPoint 3  1 0 0
  pts InsertPoint 4  1 1 0
  pts InsertPoint 5  1 2 0
  pts InsertPoint 6  2 0 0
  pts InsertPoint 7  2 2 0
  pts InsertPoint 8  3 0 0
  pts InsertPoint 9  3 1 0
  pts InsertPoint 10 3 2 0
  pts InsertPoint 11 4 0 0
  pts InsertPoint 12 6 0 0
  pts InsertPoint 13 5 2 0
  pts InsertPoint 14 7 0 0
  pts InsertPoint 15 9 0 0
  pts InsertPoint 16 7 2 0
  pts InsertPoint 17 9 2 0
  pts InsertPoint 18 10 0 0
  pts InsertPoint 19 12 0 0
  pts InsertPoint 20 10 1 0
  pts InsertPoint 21 12 1 0
  pts InsertPoint 22 10 2 0
  pts InsertPoint 23 12 2 0
  pts InsertPoint 24 10 3 0
  pts InsertPoint 25 12 3 0

vtkCellArray verts
  verts InsertNextCell 1
  verts InsertCellPoint 0
  verts InsertNextCell 1
  verts InsertCellPoint 1
  verts InsertNextCell 1
  verts InsertCellPoint 2
  verts InsertNextCell 3
  verts InsertCellPoint 3
  verts InsertCellPoint 4
  verts InsertCellPoint 5

vtkCellArray lines
  lines InsertNextCell 2
  lines InsertCellPoint 6
  lines InsertCellPoint 7
  lines InsertNextCell 3
  lines InsertCellPoint 8
  lines InsertCellPoint 9
  lines InsertCellPoint 10

vtkCellArray polys
  polys InsertNextCell 4
  polys InsertCellPoint 14
  polys InsertCellPoint 15
  polys InsertCellPoint 17
  polys InsertCellPoint 16
  polys InsertNextCell 3
  polys InsertCellPoint 11
  polys InsertCellPoint 12
  polys InsertCellPoint 13

vtkCellArray strips
  strips InsertNextCell 8
  strips InsertCellPoint 19
  strips InsertCellPoint 18
  strips InsertCellPoint 21
  strips InsertCellPoint 20
  strips InsertCellPoint 23
  strips InsertCellPoint 22
  strips InsertCellPoint 25
  strips InsertCellPoint 24

vtkFloatArray scalars
  scalars SetNumberOfTuples 26
  scalars SetTuple1 0 0
  scalars SetTuple1 1 50
  scalars SetTuple1 2 100
  scalars SetTuple1 3 0
  scalars SetTuple1 4 50
  scalars SetTuple1 5 100
  scalars SetTuple1 6 10
  scalars SetTuple1 7 90
  scalars SetTuple1 8 10
  scalars SetTuple1 9 50
  scalars SetTuple1 10 90
  scalars SetTuple1 11 10
  scalars SetTuple1 12 40
  scalars SetTuple1 13 100
  scalars SetTuple1 14 0
  scalars SetTuple1 15 60
  scalars SetTuple1 16 40
  scalars SetTuple1 17 100
  scalars SetTuple1 18 0
  scalars SetTuple1 19 25
  scalars SetTuple1 20 25
  scalars SetTuple1 21 50
  scalars SetTuple1 22 50
  scalars SetTuple1 23 75
  scalars SetTuple1 24 75
  scalars SetTuple1 25 100

vtkPolyData polyData
  polyData SetPoints pts
  polyData SetVerts verts
  polyData SetLines lines
  polyData SetPolys polys
  polyData SetStrips strips
  [polyData GetPointData] SetScalars scalars

vtkBandedPolyDataContourFilter bf
  bf SetInputData polyData
  bf GenerateValues 3 25 75
vtkPolyDataMapper mapper
  mapper SetInputConnection [bf GetOutputPort]
  mapper SetScalarModeToUseCellData
  mapper SetScalarRange 0 4
vtkActor actor
  actor SetMapper mapper

vtkIdFilter ids
  ids SetInputConnection [bf GetOutputPort]
  ids PointIdsOn
  ids CellIdsOn
  ids FieldDataOn
vtkLabeledDataMapper ldm
  ldm SetInputConnection [ids GetOutputPort]
#  ldm SetLabelFormat "%g"
  ldm SetLabelModeToLabelFieldData
vtkActor2D pointLabels
  pointLabels SetMapper ldm


# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
#ren1 AddActor2D pointLabels #for debugging only


ren1 SetBackground 0 0 0
renWin SetSize 300 80
renWin Render
[ren1 GetActiveCamera] Zoom 3
renWin Render
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


