#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot

# This tests reversing loops and other weird situations

# create planes
# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Custom polydata to test intersections. Loop are ordered in
# reverse directions.
polysData = vtk.vtkPolyData()
polysPts = vtk.vtkPoints()
polysPolys = vtk.vtkCellArray()
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
loops = vtk.vtkPolyData()
loopPts = vtk.vtkPoints()
loopPolys = vtk.vtkCellArray()
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

cookie = vtk.vtkCookieCutter()
cookie.SetInputData(polysData)
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
