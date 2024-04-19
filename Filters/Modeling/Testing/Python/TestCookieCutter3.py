#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersModeling import vtkCookieCutter
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
from vtkmodules.util.misc import vtkGetDataRoot

# This tests reversing loops and other weird situations

# create planes
# Create the RenderWindow, Renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Custom polydata to test intersections. Loop are ordered in
# reverse directions.
polysData = vtkPolyData()
polysPts = vtkPoints()
polysPolys = vtkCellArray()
polysData.SetPoints(polysPts)
polysData.SetPolys(polysPolys)

polysPts.SetNumberOfPoints(8)
polysPts.SetPoint(0, 0.0,0.0,0.0)
polysPts.SetPoint(1, 2.0,0.0,0.0)
polysPts.SetPoint(2, 2.0,2.0,0.0)
polysPts.SetPoint(3, 0.0,2.0,0.0)
polysPts.SetPoint(4, -2.0,-2.0,0.0)
polysPts.SetPoint(5,  0.0,-2.0,0.0)
polysPts.SetPoint(6,  0.0, 0.0,0.0)
polysPts.SetPoint(7, -2.0, 0.0,0.0)

polysPolys.InsertNextCell(4)
polysPolys.InsertCellPoint(0)
polysPolys.InsertCellPoint(1)
polysPolys.InsertCellPoint(2)
polysPolys.InsertCellPoint(3)
polysPolys.InsertNextCell(4)
polysPolys.InsertCellPoint(4)
polysPolys.InsertCellPoint(7)
polysPolys.InsertCellPoint(6)
polysPolys.InsertCellPoint(5)

# Create a loop for cookie cutting
loops = vtkPolyData()
loopPts = vtkPoints()
loopPolys = vtkCellArray()
loops.SetPoints(loopPts)
loops.SetPolys(loopPolys)

loopPts.SetNumberOfPoints(4)
loopPts.SetPoint(0, -1,-1,0)
loopPts.SetPoint(1,  1,-1,0)
loopPts.SetPoint(2,  1, 1,0)
loopPts.SetPoint(3, -1, 1,0)

loopPolys.InsertNextCell(4)
loopPolys.InsertCellPoint(0)
loopPolys.InsertCellPoint(1)
loopPolys.InsertCellPoint(2)
loopPolys.InsertCellPoint(3)

cookie = vtkCookieCutter()
cookie.SetInputData(polysData)
cookie.SetLoopsData(loops)
cookie.Update()

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(cookie.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)

loopMapper = vtkPolyDataMapper()
loopMapper.SetInputData(loops)

loopActor = vtkActor()
loopActor.SetMapper(loopMapper)
loopActor.GetProperty().SetColor(1,0,0)
loopActor.GetProperty().SetRepresentationToWireframe()

ren.AddActor(actor)
ren.AddActor(loopActor)

renWin.Render()
iren.Start()
