catch {load vtktcl}
## Example tests the triangulation method in vtkPolygon
#
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create some weird polygons
#
vtkPoints points
    points InsertPoint 0 0 0 0
    points InsertPoint 1 3 0 0
    points InsertPoint 2 3 10 0
    points InsertPoint 3 2 10 0
    points InsertPoint 4 2 5 0
    points InsertPoint 5 1 5 0
    points InsertPoint 6 1 10 0
    points InsertPoint 7 0 10 0

    points InsertPoint 8 10 3.5 0
    points InsertPoint 9 11 1 0
    points InsertPoint 10 13 1 0
    points InsertPoint 11 14 3.5 0
    points InsertPoint 12 12.5 6 0
    points InsertPoint 13 12 3.5 0
    points InsertPoint 14 11.5 6 0

    points InsertPoint 15 20 0 0 
    points InsertPoint 16 23 0 0
    points InsertPoint 17 23 3 0
    points InsertPoint 18 21 3 0
    points InsertPoint 19 21 1 0
    points InsertPoint 20 22 2 0
    points InsertPoint 21 22 1 0
    points InsertPoint 22 20 1 0

    points InsertPoint 23 20 5 0 
    points InsertPoint 24 23 5 0
    points InsertPoint 25 23 8 0
    points InsertPoint 26 21 8 0
    points InsertPoint 27 21 6 0
    points InsertPoint 28 22 7 0
    points InsertPoint 29 22 6 0
    points InsertPoint 30 20 6 0

vtkCellArray polys
    polys InsertNextCell 8;#number of points
    polys InsertCellPoint 0
    polys InsertCellPoint 1
    polys InsertCellPoint 2
    polys InsertCellPoint 3
    polys InsertCellPoint 4
    polys InsertCellPoint 5
    polys InsertCellPoint 6
    polys InsertCellPoint 7

    polys InsertNextCell 7
    polys InsertCellPoint 8
    polys InsertCellPoint 9
    polys InsertCellPoint 10
    polys InsertCellPoint 11
    polys InsertCellPoint 12
    polys InsertCellPoint 13
    polys InsertCellPoint 14

    polys InsertNextCell 9
    polys InsertCellPoint 15
    polys InsertCellPoint 16
    polys InsertCellPoint 17
    polys InsertCellPoint 18
    polys InsertCellPoint 19
    polys InsertCellPoint 20
    polys InsertCellPoint 21
    polys InsertCellPoint 19
    polys InsertCellPoint 22

    polys InsertNextCell 8
    polys InsertCellPoint 23
    polys InsertCellPoint 24
    polys InsertCellPoint 25
    polys InsertCellPoint 26
    polys InsertCellPoint 27
    polys InsertCellPoint 28
    polys InsertCellPoint 29
    polys InsertCellPoint 30

vtkPolyData polyData
    polyData SetPoints points
    polyData SetPolys polys

# triangulate the polygons
vtkTriangleFilter tf
    tf SetInput polyData

vtkPolyDataMapper map
    map SetInput [tf GetOutput]

vtkActor tri
    tri SetMapper map
    [tri GetProperty] SetColor 0.3800 0.7000 0.1600

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor tri
ren1 SetBackground 1 1 1
[ren1 GetActiveCamera] Dolly 1.4

renWin SetSize 500 500
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin SetFileName "triangulate.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


