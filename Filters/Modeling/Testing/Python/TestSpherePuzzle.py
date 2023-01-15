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

# prevent the tk window from showing up then start the event loop
renWin = vtkRenderWindow()
# create a rendering window and renderer
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
LastVal = -1
def MotionCallback (x,y,__vtk__temp0=0,__vtk__temp1=0):
    global LastVal
    # Compute display point from Tk display point.
    WindowY = 400
    y = expr.expr(globals(), locals(),["WindowY","-","y"])
    z = ren1.GetZ(x,y)
    ren1.SetDisplayPoint(x,y,z)
    ren1.DisplayToWorld()
    pt = ren1.GetWorldPoint()
    #tk_messageBox -message "$pt"
    x = lindex(pt,0)
    y = lindex(pt,1)
    z = lindex(pt,2)
    val = puzzle.SetPoint(x,y,z)
    if (val != LastVal):
        renWin.Render()
        LastVal = val

def ButtonCallback (x,y,__vtk__temp0=0,__vtk__temp1=0):
    # Compute display point from Tk display point.
    WindowY = 400
    y = expr.expr(globals(), locals(),["WindowY","-","y"])
    z = ren1.GetZ(x,y)
    ren1.SetDisplayPoint(x,y,z)
    ren1.DisplayToWorld()
    pt = ren1.GetWorldPoint()
    #tk_messageBox -message "$pt"
    x = lindex(pt,0)
    y = lindex(pt,1)
    z = lindex(pt,2)
    # Had to move away from mouse events (sgi RT problems)
    i = 0
    while i <= 100:
        puzzle.SetPoint(x,y,z)
        puzzle.MovePoint(i)
        renWin.Render()
        i = expr.expr(globals(), locals(),["i","+","5"])


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
