#!/usr/bin/env python
from vtkmodules.vtkFiltersModeling import (
    vtkSpherePuzzle,
    vtkSpherePuzzleArrows,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window and renderer
renWin = vtkRenderWindow()
ren1 = vtkRenderer()
renWin.AddRenderer(ren1)
renWin.SetSize(400,400)
puzzle = vtkSpherePuzzle()
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(puzzle.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mapper)
arrows = vtkSpherePuzzleArrows()
mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(arrows.GetOutputPort())
actor2 = vtkActor()
actor2.SetMapper(mapper2)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(actor2)
ren1.SetBackground(0.1,0.2,0.4)
renWin.Render()
cam = ren1.GetActiveCamera()
cam.Elevation(-40)
puzzle.MoveHorizontal(0,100,0)
puzzle.MoveHorizontal(1,100,1)
puzzle.MoveHorizontal(2,100,0)
puzzle.MoveVertical(2,100,0)
puzzle.MoveVertical(1,100,0)
renWin.Render()
# --- end of script --
