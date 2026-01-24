#!/usr/bin/env python
from vtkmodules.vtkFiltersMeshing import (
    vtkVoronoiHull
    )
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkIOLegacy import vtkPolyDataWriter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

# Construct an octahedron with the bottom vertex cut off,
# and the top vertex a slight (degenerate) graze. This tests
# self-degeneracies, and prunes.
center = [0.0,0.0,0.0]
bds = [-10.0,10.0, -10.0,10.0, -10.0,10.0]

hull = vtkVoronoiHull()
hull.Initialize(0, center, bds)

hull.Clip(1, [-1,-1,-0.75])
hull.Clip(2, [ 1,-1,-0.75])
hull.Clip(3, [ 1, 1,-0.75])
hull.Clip(4, [-1, 1,-0.75])

hull.Clip(5, [-1,-1, 0.75])
hull.Clip(6, [ 1,-1, 0.75])
hull.Clip(7, [ 1, 1, 0.75])
hull.Clip(8, [-1, 1, 0.75])

hull.Clip(9, [0,0,-1])
pruned = hull.Clip(10, [0,0,3.4166666649591])
if pruned == 2:
    print("Pruning occured")
    assert(pruned == 2)

pd = vtkPolyData()
hull.ProducePolyData(pd)

print("Output hull")
print("\tNumber of points:",pd.GetNumberOfPoints())
print("\tNumber of faces:",pd.GetNumberOfCells())

mapper = vtkPolyDataMapper()
mapper.SetInputData(pd)

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0.8,0.8,0.9)
actor.GetProperty().EdgeVisibilityOn()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(500,500)
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
