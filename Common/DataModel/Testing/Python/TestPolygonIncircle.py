#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkCommonCore import (
    vtkIdList,
    vtkPoints,
    reference
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
    vtkPolygon
)
from vtkmodules.vtkFiltersSources import vtkRegularPolygonSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkProperty,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(450, 150)
iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);

# Create three cells -- triangle, quad, pentagon -- and
# place the incircle inside of each. Only the triangle
# and pentagon are regular.

# Regular triangle polygon
triSource = vtkRegularPolygonSource()
triSource.SetNumberOfSides(3)
triSource.SetCenter(-2.5,0,0)
triSource.SetRadius(1)
triSource.Update()

triMapper = vtkPolyDataMapper()
triMapper.SetInputConnection(triSource.GetOutputPort())

triActor = vtkActor()
triActor.SetMapper(triMapper)
triActor.GetProperty().SetRepresentationToWireframe()

# Non-regular quad polygon
quadPts = vtkPoints()
quadPts.SetNumberOfPoints(4)
quadPts.SetPoint(0,-1,-0.75,0)
quadPts.SetPoint(1, 1,-0.75,0)
quadPts.SetPoint(2, 1, 0.75,0)
quadPts.SetPoint(3,-1, 0.75,0)

quadPolygon = vtkPolygon()
quadPolygon.GetPointIds().SetNumberOfIds(4)
quadPolygon.GetPointIds().SetId(0,0)
quadPolygon.GetPointIds().SetId(1,1)
quadPolygon.GetPointIds().SetId(2,2)
quadPolygon.GetPointIds().SetId(3,3)

quadCells = vtkCellArray()
quadCells.InsertNextCell(quadPolygon)

quadPolyData = vtkPolyData()
quadPolyData.SetPoints(quadPts)
quadPolyData.SetPolys(quadCells)

quadMapper = vtkPolyDataMapper()
quadMapper.SetInputData(quadPolyData)

quadActor = vtkActor()
quadActor.SetMapper(quadMapper)
quadActor.GetProperty().SetRepresentationToWireframe()

# Regular pentagon polygon
pentSource = vtkRegularPolygonSource()
pentSource.SetNumberOfSides(5)
pentSource.SetCenter(2.5,0,0)
pentSource.SetRadius(1)
pentSource.Update()

pentMapper = vtkPolyDataMapper()
pentMapper.SetInputConnection(pentSource.GetOutputPort())

pentActor = vtkActor()
pentActor.SetMapper(pentMapper)
pentActor.GetProperty().SetRepresentationToWireframe()

# Now generate incircles
cellSize = reference(0)
radius2 = reference(0.0)
center = [0.0, 0.0, 0.0]
t = reference((0,))
cellIds = vtkIdList()

triCircle = vtkRegularPolygonSource()
triCircle.SetNumberOfSides(64)

triCircleMapper = vtkPolyDataMapper()
triCircleMapper.SetInputConnection(triCircle.GetOutputPort())

triCircleActor = vtkActor()
triCircleActor.SetMapper(triCircleMapper)
triCircleActor.GetProperty().SetRepresentationToWireframe()
triCircleActor.GetProperty().SetColor(1,0,0)

triPts = triSource.GetOutput().GetPoints()
triCells = triSource.GetOutput().GetPolys()
triCell = triCells.GetCellAtId(0,cellIds)
cellSize = cellIds.GetNumberOfIds()
pts = [i for i in range(cellSize)]

triPolygon = vtkPolygon()
triPolygon.ComputeInteriorCircle(triPts,cellSize,pts,center,radius2)
triCircle.SetCenter(center)
triCircle.SetRadius(radius2**0.5)
triCircleActor.GetProperty().SetLineWidth(4)
triCircleActor.AddPosition(0,0,0.01) #avoid zbuffering issues

quadCircle = vtkRegularPolygonSource()
quadCircle.SetNumberOfSides(64)

quadCircleMapper = vtkPolyDataMapper()
quadCircleMapper.SetInputConnection(quadCircle.GetOutputPort())

quadCircleActor = vtkActor()
quadCircleActor.SetMapper(quadCircleMapper)
quadCircleActor.GetProperty().SetRepresentationToWireframe()
quadCircleActor.GetProperty().SetColor(1,0,0)
quadCircleActor.GetProperty().SetLineWidth(4)
quadCircleActor.AddPosition(0,0,0.01) #avoid zbuffering issues

cellSize = quadPolygon.GetPointIds().GetNumberOfIds()
pts = [i for i in range(cellSize)]

quadPolygon.ComputeInteriorCircle(quadPts,cellSize,pts,center,radius2)
quadCircle.SetCenter(center)
quadCircle.SetRadius(radius2**0.5)

pentCircle = vtkRegularPolygonSource()
pentCircle.SetNumberOfSides(64)

pentCircleMapper = vtkPolyDataMapper()
pentCircleMapper.SetInputConnection(pentCircle.GetOutputPort())

pentCircleActor = vtkActor()
pentCircleActor.SetMapper(pentCircleMapper)
pentCircleActor.GetProperty().SetRepresentationToWireframe()
pentCircleActor.GetProperty().SetColor(1,0,0)
pentCircleActor.GetProperty().SetLineWidth(4)
pentCircleActor.AddPosition(0,0,0.01) #avoid zbuffering issues

pentPts = pentSource.GetOutput().GetPoints()
pentCells = pentSource.GetOutput().GetPolys()
pentCell = pentCells.GetCellAtId(0,cellIds)
cellSize = cellIds.GetNumberOfIds()
pts = [i for i in range(cellSize)]

pentPolygon = vtkPolygon()
pentPolygon.ComputeInteriorCircle(pentPts,cellSize,pts,center,radius2)
pentCircle.SetCenter(center)
pentCircle.SetRadius(radius2**0.5)

# Display
ren.AddActor(triActor)
ren.AddActor(triCircleActor)

ren.AddActor(quadActor)
ren.AddActor(quadCircleActor)

ren.AddActor(pentActor)
ren.AddActor(pentCircleActor)

ren.ResetCamera()
ren.GetActiveCamera().Zoom(2.5)
renWin.Render()
iRen.Start()
