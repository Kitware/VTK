#!/usr/bin/env python
import sys
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCellPicker,
    vtkHardwarePicker,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

# Quad
pts = vtkPoints()
pts.SetNumberOfPoints(6)
coords = [(0, 0, 0), (2, 0, 0), (4, 0, 0), (0, 4, 0), (2, 4, 0), (4, 4, 0)]
for i in range(0, 6):
    pts.InsertPoint(i, coords[i])
quads = vtkCellArray()
cellpoints = [(0, 1, 4, 3), (1, 2, 5, 4)]
for i in range(0, 2):
    quads.InsertNextCell(4)
    for j in range(0, 4):
        quads.InsertCellPoint(cellpoints[i][j])

poly = vtkPolyData()
poly.SetPoints(pts)
poly.SetPolys(quads)

mapper = vtkPolyDataMapper()
mapper.SetInputData(poly)

actor = vtkActor()
actor.SetMapper(mapper)

ren = vtkRenderer()
ren.AddActor(actor)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(200, 200)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.Render()
iren.Initialize()

pos = [(1, 1), (35, 40), (80, 40), (120, 40), (165, 40),
       (199, 160), (165, 160), (120, 160), (80, 160), (35, 160)]
pickedCells = [-1, 0, 0, 1, 1,
               -1, 1, 1, 0, 0]
pickedPoints = [-1, 0, 1, 1, 2,
                -1, 5, 4, 4, 3]

cellErrorsCP = 0
pointErrorsCP = 0
cellPicker = vtkCellPicker()
cellErrorsHP = 0
pointErrorsHP = 0
hardwarePicker = vtkHardwarePicker()
for i in range(0, len(pos)):
    # Check cell picker
    cellPicker.Pick(pos[i][0], pos[i][1], 0, ren)
    if cellPicker.GetCellId() != pickedCells[i]:
        print("pos [{0}] : picked cell id={1} expected cell id={2}".format(pos[i], cellPicker.GetCellId(),
                                                                           pickedCells[i]))
        cellErrorsCP = cellErrorsCP + 1
    if cellPicker.GetPointId() != pickedPoints[i]:
        print("pos [{0}] : picked point id={1} expected point id={2}".format(pos[i], cellPicker.GetPointId(),
                                                                             pickedPoints[i]))
        pointErrorsCP = pointErrorsCP + 1
    # Check hardware picker
    hardwarePicker.SnapToMeshPointOff()
    hardwarePicker.Pick(pos[i][0], pos[i][1], 0, ren)
    if hardwarePicker.GetCellId() != pickedCells[i]:
        print("pos [{0}] : picked cell id={1} expected cell id={2}".format(pos[i], hardwarePicker.GetCellId(),
                                                                           pickedCells[i]))
        cellErrorsHP = cellErrorsHP + 1
    hardwarePicker.SnapToMeshPointOn()
    hardwarePicker.SetPixelTolerance(20)
    hardwarePicker.Pick(pos[i][0], pos[i][1], 0, ren)
    if hardwarePicker.GetPointId() != pickedPoints[i]:
        print("pos [{0}] : picked point id={1} expected point id={2}".format(pos[i], hardwarePicker.GetPointId(),
                                                                             pickedPoints[i]))
        pointErrorsHP = pointErrorsHP + 1

if pointErrorsCP or cellErrorsCP:
    print("ERROR: Cell picker : {0} cell-id errors, {1} point-id errors".format(cellErrorsCP, pointErrorsCP))
    sys.exit(1)

if pointErrorsHP or cellErrorsHP:
    print("ERROR: Hardware picker : {0} cell-id errors, {1} point-id errors".format(cellErrorsHP, pointErrorsHP))
    sys.exit(1)
