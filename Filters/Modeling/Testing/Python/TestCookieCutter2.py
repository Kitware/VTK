#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkStripper,
    vtkTriangleFilter,
)
from vtkmodules.vtkFiltersModeling import vtkCookieCutter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
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

# This tests special cases like polylines and triangle strips

# create planes
# Create the RenderWindow, Renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Custom polydata to test polylines
linesData = vtkPolyData()
linesPts = vtkPoints()
linesLines = vtkCellArray()
linesData.SetPoints(linesPts)
linesData.SetLines(linesLines)

linesPts.InsertPoint(0,  -1.0,-1.0, 0.0)
linesPts.InsertPoint(1,   1.0,-1.0, 0.0)
linesPts.InsertPoint(2,   1.0, 1.0, 0.0)
linesPts.InsertPoint(3,  -1.0, 1.0, 0.0)
linesPts.InsertPoint(4,  -0.2,-0.2, 0.0)
linesPts.InsertPoint(5,   0.0, 0.0, 0.0)
linesPts.InsertPoint(6,   0.2, 0.2, 0.0)

linesLines.InsertNextCell(3)
linesLines.InsertCellPoint(1)
linesLines.InsertCellPoint(5)
linesLines.InsertCellPoint(3)
linesLines.InsertNextCell(5)
linesLines.InsertCellPoint(0)
linesLines.InsertCellPoint(4)
linesLines.InsertCellPoint(5)
linesLines.InsertCellPoint(6)
linesLines.InsertCellPoint(2)

# Create a triangle strip to cookie cut
plane = vtkPlaneSource()
plane.SetXResolution(25)
plane.SetYResolution(1)
plane.SetOrigin(-1,-0.1,0)
plane.SetPoint1( 1,-0.1,0)
plane.SetPoint2(-1, 0.1,0)

tri = vtkTriangleFilter()
tri.SetInputConnection(plane.GetOutputPort())

stripper = vtkStripper()
stripper.SetInputConnection(tri.GetOutputPort())

append = vtkAppendPolyData()
append.AddInputData(linesData)
append.AddInputConnection(stripper.GetOutputPort())

# Create a loop for cookie cutting
loops = vtkPolyData()
loopPts = vtkPoints()
loopPolys = vtkCellArray()
loops.SetPoints(loopPts)
loops.SetPolys(loopPolys)

loopPts.SetNumberOfPoints(4)
loopPts.SetPoint(0, -0.35,0.0,0.0)
loopPts.SetPoint(1, 0,-0.35,0.0)
loopPts.SetPoint(2, 0.35,0.0,0.0)
loopPts.SetPoint(3, 0.0,0.35,0.0)

loopPolys.InsertNextCell(4)
loopPolys.InsertCellPoint(0)
loopPolys.InsertCellPoint(1)
loopPolys.InsertCellPoint(2)
loopPolys.InsertCellPoint(3)

cookie = vtkCookieCutter()
cookie.SetInputConnection(append.GetOutputPort())
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
