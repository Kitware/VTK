package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPoints points
    points InsertPoint 0 0.0 0.0 0.0

vtkCellArray verts
    verts InsertNextCell 1
    verts InsertCellPoint 0

vtkPolyData polyData
    polyData SetPoints points
    polyData SetVerts verts

vtkPolyDataMapper mapper
    mapper SetInput polyData

vtkActor actor
    actor SetMapper mapper
    [actor GetProperty] SetPointSize 8

ren1 AddProp actor

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .





