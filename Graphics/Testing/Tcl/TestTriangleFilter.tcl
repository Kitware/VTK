package require vtk
package require vtkinteraction

vtkPoints pts
pts SetNumberOfPoints 22
pts SetPoint 0 0 0 0
pts SetPoint 1 1 0 0
pts SetPoint 2 2 0 0
pts SetPoint 3 3 0 0
pts SetPoint 4 4 0 0
pts SetPoint 5 5 0 0
pts SetPoint 6 6 0 0
pts SetPoint 7 7 0 0
pts SetPoint 8 8 0 0
pts SetPoint 9 9 0 0
pts SetPoint 10 1 1 0
pts SetPoint 11 2 1 0
pts SetPoint 12 3 1 0
pts SetPoint 13 4 1 0
pts SetPoint 14 6 1 0
pts SetPoint 15 8 1 0
pts SetPoint 16 9 1 0
pts SetPoint 17 3 2 0
pts SetPoint 18 6 2 0
pts SetPoint 19 7 2 0
pts SetPoint 20 8 2 0
pts SetPoint 21 9 2 0

vtkPolyData pd
pd SetPoints pts
vtkCellArray verts
verts InsertNextCell 1
verts InsertCellPoint 0
verts InsertNextCell 2
verts InsertCellPoint 1
verts InsertCellPoint 10
pd SetVerts verts

vtkCellArray lines
lines InsertNextCell 2
lines InsertCellPoint 2
lines InsertCellPoint 11
lines InsertNextCell 3
lines InsertCellPoint 3
lines InsertCellPoint 12
lines InsertCellPoint 17
pd SetLines lines

vtkCellArray polys
polys InsertNextCell 3
polys InsertCellPoint 4
polys InsertCellPoint 5
polys InsertCellPoint 13
polys InsertNextCell 5
polys InsertCellPoint 6
polys InsertCellPoint 7
polys InsertCellPoint 19
polys InsertCellPoint 18
polys InsertCellPoint 14
pd SetPolys polys

vtkCellArray strips
strips InsertNextCell 6
strips InsertCellPoint 8
strips InsertCellPoint 9
strips InsertCellPoint 15
strips InsertCellPoint 16
strips InsertCellPoint 20
strips InsertCellPoint 21
pd SetStrips strips

vtkUnsignedCharArray colors
colors SetNumberOfComponents 4
colors SetNumberOfTuples 7
colors SetTuple4 0 255 0 0 255
colors SetTuple4 1 0 255 0 255
colors SetTuple4 2 0 0 255 255
colors SetTuple4 3 255 255 0 255
colors SetTuple4 4 255 0 255 255
colors SetTuple4 5 0 255 0 255
colors SetTuple4 6 0 255 255 255
[pd GetCellData] SetScalars colors

vtkTriangleFilter tf
tf SetInputData pd

vtkPolyDataMapper mapper
mapper SetInputConnection [tf GetOutputPort]

vtkActor actor
actor SetMapper mapper

# Create the RenderWindow, Renderer and interactive renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
ren1 SetBackground 1 1 1

renWin SetSize 300 150
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 2.5
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
