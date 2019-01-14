#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


def buildPolyDataLine(pt1, pt2):
    pd = vtk.vtkPolyData()

    pts = vtk.vtkPoints()
    pts.InsertNextPoint(pt1)
    pts.InsertNextPoint(pt2)

    pd.SetPoints(pts)

    lines = vtk.vtkCellArray()
    lines.InsertNextCell(2)
    lines.InsertCellPoint(0)
    lines.InsertCellPoint(1)

    pd.SetLines(lines)

    return pd


def buildPolyDataItemMixed():
    ptdata = [
        [0.6,  0.6,  0.0],
        [0.75, 0.6,  0.0],
        [0.9,  0.6,  0.0],
        [0.6,  0.75, 0.0],
        [0.75, 0.75, 0.0],
        [0.9,  0.75, 0.0],
        [0.6,  0.9,  0.0],
        [0.75, 0.9,  0.0],
        [0.9,  0.9, 0.0]
    ]

    pd = vtk.vtkPolyData()
    pd.Allocate(50)

    pts = vtk.vtkPoints()

    for pt in ptdata:
        pts.InsertNextPoint(pt)

    pd.SetPoints(pts)

    pd.InsertNextCell(vtk.VTK_POLY_LINE, 3, [3, 6, 7])
    pd.InsertNextCell(vtk.VTK_POLY_LINE, 2, [8, 5])
    pd.InsertNextCell(vtk.VTK_TRIANGLE,  3, [5, 2, 1])
    pd.InsertNextCell(vtk.VTK_POLY_LINE, 3, [1, 4, 5])
    pd.InsertNextCell(vtk.VTK_TRIANGLE,  3, [0, 3, 4])

    colors = vtk.vtkUnsignedCharArray()
    colors.SetNumberOfComponents(4)
    colors.SetNumberOfTuples(5)

    colors.SetTuple4(0, *[255, 0, 0, 255])
    colors.SetTuple4(1, *[0, 255, 0, 255])
    colors.SetTuple4(2, *[0, 0, 255, 255])
    colors.SetTuple4(3, *[255, 255, 0, 255])
    colors.SetTuple4(4, *[0, 255, 255, 255])

    return pd, colors


def buildPolyDataTriangle(pt1, pt2, pt3):
    pd = vtk.vtkPolyData()

    pts = vtk.vtkPoints()
    pts.InsertNextPoint(pt1)
    pts.InsertNextPoint(pt2)
    pts.InsertNextPoint(pt3)

    pd.SetPoints(pts)

    tris = vtk.vtkCellArray()
    tris.InsertNextCell(3)
    tris.InsertCellPoint(0)
    tris.InsertCellPoint(1)
    tris.InsertCellPoint(2)

    pd.SetPolys(tris)

    return pd


def buildPolyDataLineStrips(ptsList):
    pd = vtk.vtkPolyData()

    pts = vtk.vtkPoints()
    lines = vtk.vtkCellArray()

    ptIdx = 0

    for i in range(len(ptsList)):
        lineStrip = ptsList[i]
        numCellPoints = len(lineStrip)
        lines.InsertNextCell(numCellPoints)
        for j in range(numCellPoints):
            pt = lineStrip[j]
            pts.InsertNextPoint(pt)
            lines.InsertCellPoint(ptIdx)
            ptIdx += 1

    pd.SetPoints(pts)
    pd.SetLines(lines)

    return pd


polyDataList = [
    {
        # A red line
        'poly': buildPolyDataLine([0.5, 0.5, 0.0], [0.0, 1.0, 0.0]),
        'color': [255, 0, 0, 255]
    },
    {
        # A green triangle
        'poly': buildPolyDataTriangle([0.5, 0.0, 0.0], [0.75, 0.5, 0.0], [1.0, 0.0, 0.0]),
        'color': [0, 255, 0, 255]
    },
    {
        # A blue triangle in [0, 0] - [0.5, 0.5]
        'poly': buildPolyDataLineStrips([[
            [0.1, 0.1, 0.0],
            [0.1, 0.4, 0.0],
            [0.4, 0.4, 0.0],
            [0.4, 0.2, 0.0],
            [0.2, 0.2, 0.0],
            [0.2, 0.3, 0.0],
            [0.3, 0.3, 0.0]
        ]]),
        'color': [0, 0, 255, 255]
    }
]

width = 400
height = 400

view = vtk.vtkContextView()
renWin = view.GetRenderWindow()
renWin.SetSize(width, height)

area = vtk.vtkInteractiveArea()
view.GetScene().AddItem(area)

drawAreaBounds = vtk.vtkRectd(0.0, 0.0, 1.0, 1.0)

vp = [0.0500000007451, 0.949999988079, 0.259999990463, 0.860000014305]
screenGeometry = vtk.vtkRecti(int(vp[0] * width),
                              int(vp[2] * height),
                              int((vp[1] - vp[0]) * width),
                              int((vp[3] - vp[2]) * height))

for obj in polyDataList:
    pd = obj['poly']
    col = obj['color']

    item = vtk.vtkPolyDataItem()
    item.SetPolyData(pd)
    item.SetScalarMode(vtk.VTK_SCALAR_MODE_USE_CELL_DATA)
    mappedColors = vtk.vtkUnsignedCharArray()
    mappedColors.SetNumberOfComponents(4)
    mappedColors.SetNumberOfTuples(pd.GetNumberOfCells())

    for i in range(pd.GetNumberOfCells()):
        mappedColors.SetTuple4(i, *col)

    item.SetMappedColors(mappedColors)
    item.SetVisible(True)
    area.GetDrawAreaItem().AddItem(item)

lastItem = vtk.vtkPolyDataItem()
pd, colors = buildPolyDataItemMixed()
lastItem.SetPolyData(pd)
lastItem.SetScalarMode(vtk.VTK_SCALAR_MODE_USE_CELL_DATA)
lastItem.SetMappedColors(colors)
lastItem.SetVisible(True)
area.GetDrawAreaItem().AddItem(lastItem)

area.SetDrawAreaBounds(drawAreaBounds)
area.SetGeometry(screenGeometry)
area.SetFillViewport(False)
area.SetShowGrid(False)

axisLeft = area.GetAxis(vtk.vtkAxis.LEFT)
axisRight = area.GetAxis(vtk.vtkAxis.RIGHT)
axisBottom = area.GetAxis(vtk.vtkAxis.BOTTOM)
axisTop = area.GetAxis(vtk.vtkAxis.TOP)
axisTop.SetVisible(False)
axisRight.SetVisible(False)
axisLeft.SetVisible(False)
axisBottom.SetVisible(False)
axisTop.SetMargins(0, 0)
axisRight.SetMargins(0, 0)
axisLeft.SetMargins(0, 0)
axisBottom.SetMargins(0, 0)

renWin.Render()

# iren = view.GetInteractor()
# iren.Initialize()
# iren.GetInteractorStyle().SetCurrentRenderer(view.GetRenderer());
# iren.Start()
