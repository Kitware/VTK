# This example demonstrates how to use a constraint polygon in Delaunay 
# triangulation.
#
package require vtk
package require vtkinteraction
package require vtktesting

# Generate the input points and constrained edges/polygons.
#
vtkPoints points
    points InsertPoint 0 1 4 0
    points InsertPoint 1 3 4 0
    points InsertPoint 2 7 4 0
    points InsertPoint 3 11 4 0
    points InsertPoint 4 13 4 0
    points InsertPoint 5 13 8 0
    points InsertPoint 6 13 12 0
    points InsertPoint 7 10 12 0
    points InsertPoint 8 7 12 0
    points InsertPoint 9 4 12 0
    points InsertPoint 10 1 12 0
    points InsertPoint 11 1 8 0
    points InsertPoint 12 3.5 5 0
    points InsertPoint 13 4.5 5 0
    points InsertPoint 14 5.5 8 0
    points InsertPoint 15 6.5 8 0
    points InsertPoint 16 6.5 5 0
    points InsertPoint 17 7.5 5 0
    points InsertPoint 18 7.5 8 0
    points InsertPoint 19 9 8 0
    points InsertPoint 20 9 5 0
    points InsertPoint 21 10 5 0
    points InsertPoint 22 10 7 0
    points InsertPoint 23 11 5 0
    points InsertPoint 24 12 5 0
    points InsertPoint 25 10.5 8 0
    points InsertPoint 26 12 11 0
    points InsertPoint 27 11 11 0
    points InsertPoint 28 10 9 0
    points InsertPoint 29 10 11 0
    points InsertPoint 30 9 11 0
    points InsertPoint 31 9 9 0
    points InsertPoint 32 7.5 9 0
    points InsertPoint 33 7.5 11 0
    points InsertPoint 34 6.5 11 0
    points InsertPoint 35 6.5 9 0
    points InsertPoint 36 5 9 0
    points InsertPoint 37 4 6 0
    points InsertPoint 38 3 9 0
    points InsertPoint 39 2 9 0
vtkCellArray polys
    polys InsertNextCell 12
    polys InsertCellPoint 0
    polys InsertCellPoint 1
    polys InsertCellPoint 2
    polys InsertCellPoint 3
    polys InsertCellPoint 4
    polys InsertCellPoint 5
    polys InsertCellPoint 6
    polys InsertCellPoint 7
    polys InsertCellPoint 8
    polys InsertCellPoint 9
    polys InsertCellPoint 10
    polys InsertCellPoint 11
    polys InsertNextCell 28
    polys InsertCellPoint 39
    polys InsertCellPoint 38
    polys InsertCellPoint 37
    polys InsertCellPoint 36
    polys InsertCellPoint 35
    polys InsertCellPoint 34
    polys InsertCellPoint 33
    polys InsertCellPoint 32
    polys InsertCellPoint 31
    polys InsertCellPoint 30
    polys InsertCellPoint 29
    polys InsertCellPoint 28
    polys InsertCellPoint 27
    polys InsertCellPoint 26
    polys InsertCellPoint 25
    polys InsertCellPoint 24
    polys InsertCellPoint 23
    polys InsertCellPoint 22
    polys InsertCellPoint 21
    polys InsertCellPoint 20
    polys InsertCellPoint 19
    polys InsertCellPoint 18
    polys InsertCellPoint 17
    polys InsertCellPoint 16
    polys InsertCellPoint 15
    polys InsertCellPoint 14
    polys InsertCellPoint 13
    polys InsertCellPoint 12

vtkPolyData polyData
    polyData SetPoints points
    polyData SetPolys polys

# Notice this trick. The SetInput() method accepts a vtkPolyData that is also
# the input to the Delaunay filter. The points of the vtkPolyData are used
# to generate the triangulation; the polygons are used to create a constraint
# region. The polygons are very carefully created and ordered in the right
# direction to indicate inside and outside of the polygon.
#
vtkDelaunay2D del
    del SetInput polyData
    del SetSource polyData
vtkPolyDataMapper mapMesh
    mapMesh SetInput [del GetOutput]
vtkActor meshActor
    meshActor SetMapper mapMesh

# Now we just pretty the mesh up with tubed edges and balls at the vertices.
vtkExtractEdges extract
    extract SetInput [del GetOutput]
vtkTubeFilter tubes
    tubes SetInput [extract GetOutput]
    tubes SetRadius 0.1
    tubes SetNumberOfSides 6
vtkPolyDataMapper mapEdges
    mapEdges SetInput [tubes GetOutput]
vtkActor edgeActor
    edgeActor SetMapper mapEdges
    eval [edgeActor GetProperty] SetColor $peacock
    [edgeActor GetProperty] SetSpecularColor 1 1 1
    [edgeActor GetProperty] SetSpecular 0.3
    [edgeActor GetProperty] SetSpecularPower 20
    [edgeActor GetProperty] SetAmbient 0.2
    [edgeActor GetProperty] SetDiffuse 0.8

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor meshActor
ren1 AddActor edgeActor
ren1 SetBackground 0 0 0
renWin SetSize 450 300

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 2
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
