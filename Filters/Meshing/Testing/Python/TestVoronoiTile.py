#!/usr/bin/env python
from vtkmodules.vtkFiltersMeshing import (
    vtkVoronoiTile
    )
from vtkmodules.vtkCommonDataModel import vtkPolyData
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

# Construct a diamond with the bottom vertex cut off,
# and the top vertex a slight (degenerate) graze.
center = [0.0,0.0,0.0]
bds = [-10.0,10.0, -10.0,10.0]

tile = vtkVoronoiTile()
tile.Initialize(0, center, bds)

tile.Clip(1, [-1,-1])
tile.Clip(2, [ 1,-1])
tile.Clip(3, [ 1, 1])
tile.Clip(4, [-1, 1])
tile.Clip(5, [ 0,-1.5])
pruned = tile.Clip(6, [ 0, 1.999999999999999])
if pruned == 2:
    print("Pruning occured")
    assert(pruned == 2)

pd = vtkPolyData()
tile.ProducePolyData(pd)

numPts = pd.GetNumberOfPoints()
print("Output tile")
print("\tNumber of points/edges:",numPts)
assert(numPts == 5)

mapper = vtkPolyDataMapper()
mapper.SetInputData(pd)

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()

iren.Initialize()
# --- end of script --
