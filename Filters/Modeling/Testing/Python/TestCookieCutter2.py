#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot

# This tests special cases like polylines and triangle strips

# create planes
# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Custom polydata to test polylines
linesData = vtk.vtkPolyData()
linesPts = vtk.vtkPoints()
linesLines = vtk.vtkCellArray()
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
plane = vtk.vtkPlaneSource()
plane.SetXResolution(25)
plane.SetYResolution(1)
plane.SetOrigin(-1,-0.1,0)
plane.SetPoint1( 1,-0.1,0)
plane.SetPoint2(-1, 0.1,0)

tri = vtk.vtkTriangleFilter()
tri.SetInputConnection(plane.GetOutputPort())

stripper = vtk.vtkStripper()
stripper.SetInputConnection(tri.GetOutputPort())

append = vtk.vtkAppendPolyData()
append.AddInputData(linesData)
append.AddInputConnection(stripper.GetOutputPort())

# Create a loop for cookie cutting
loops = vtk.vtkPolyData()
loopPts = vtk.vtkPoints()
loopPolys = vtk.vtkCellArray()
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

cookie = vtk.vtkCookieCutter()
cookie.SetInputConnection(append.GetOutputPort())
cookie.SetLoopsData(loops)
cookie.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(cookie.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

loopMapper = vtk.vtkPolyDataMapper()
loopMapper.SetInputData(loops)

loopActor = vtk.vtkActor()
loopActor.SetMapper(loopMapper)
loopActor.GetProperty().SetColor(1,0,0)
loopActor.GetProperty().SetRepresentationToWireframe()

ren.AddActor(actor)
ren.AddActor(loopActor)

renWin.Render()
#iren.Start()
