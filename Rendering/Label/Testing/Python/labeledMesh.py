#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import (
    vtkCellCenters,
    vtkGenerateIds,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkActor2D,
    vtkPolyDataMapper,
    vtkPolyDataMapper2D,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkSelectVisiblePoints,
)
from vtkmodules.vtkRenderingLabel import vtkLabeledDataMapper
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# demonstrate use of point labeling and the selection window

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a selection window
xmin = 200
xLength = 100
xmax = xmin + xLength
ymin = 200
yLength = 100
ymax = ymin + yLength

pts = vtkPoints()
pts.InsertPoint(0, xmin, ymin, 0)
pts.InsertPoint(1, xmax, ymin, 0)
pts.InsertPoint(2, xmax, ymax, 0)
pts.InsertPoint(3, xmin, ymax, 0)

rect = vtkCellArray()
rect.InsertNextCell(5)
rect.InsertCellPoint(0)
rect.InsertCellPoint(1)
rect.InsertCellPoint(2)
rect.InsertCellPoint(3)
rect.InsertCellPoint(0)

selectRect = vtkPolyData()
selectRect.SetPoints(pts)
selectRect.SetLines(rect)

rectMapper = vtkPolyDataMapper2D()
rectMapper.SetInputData(selectRect)

rectActor = vtkActor2D()
rectActor.SetMapper(rectMapper)

# Create asphere
sphere = vtkSphereSource()

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)

# Generate ids for labeling
ids = vtkGenerateIds()
ids.SetInputConnection(sphere.GetOutputPort())
ids.PointIdsOn()
ids.CellIdsOn()
ids.FieldDataOn()

# Create labels for points
visPts = vtkSelectVisiblePoints()
visPts.SetInputConnection(ids.GetOutputPort())
visPts.SetRenderer(ren1)
visPts.SelectionWindowOn()
visPts.SetSelection(xmin, xmin + xLength, ymin, ymin + yLength)

ldm = vtkLabeledDataMapper()
ldm.SetInputConnection(visPts.GetOutputPort())
#    ldm.SetLabelFormat.("%g")
#    ldm.SetLabelModeToLabelScalars()
#    ldm.SetLabelModeToLabelNormals()
ldm.SetLabelModeToLabelFieldData()
#    ldm.SetLabeledComponent(0)
ldm.SetFieldDataName("vtkPointIds");

pointLabels = vtkActor2D()
pointLabels.SetMapper(ldm)

# Create labels for cells
cc = vtkCellCenters()
cc.SetInputConnection(ids.GetOutputPort())

visCells = vtkSelectVisiblePoints()
visCells.SetInputConnection(cc.GetOutputPort())
visCells.SetRenderer(ren1)
visCells.SelectionWindowOn()
visCells.SetSelection(xmin, xmin + xLength, ymin, ymin + yLength)

cellMapper = vtkLabeledDataMapper()
cellMapper.SetInputConnection(visCells.GetOutputPort())
#    cellMapper.SetLabelFormat("%g")
#    cellMapper.SetLabelModeToLabelScalars()
#    cellMapper.SetLabelModeToLabelNormals()
cellMapper.SetLabelModeToLabelFieldData()
cellMapper.SetFieldDataName("vtkCellIds");
cellMapper.GetLabelTextProperty().SetColor(0, 1, 0)

cellLabels = vtkActor2D()
cellLabels.SetMapper(cellMapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(sphereActor)
ren1.AddViewProp(rectActor)
ren1.AddViewProp(pointLabels)
ren1.AddViewProp(cellLabels)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(500, 500)

renWin.Render()
# render the image
#

def PlaceWindow (xmin, ymin):
    global xLength, yLength
    xmax = xmin + xLength
    ymax = ymin + yLength
    visPts.SetSelection(xmin, xmax, ymin, ymax)
    visCells.SetSelection(xmin, xmax, ymin, ymax)
    pts.InsertPoint(0, xmin, ymin, 0)
    pts.InsertPoint(1, xmax, ymin, 0)
    pts.InsertPoint(2, xmax, ymax, 0)
    pts.InsertPoint(3, xmin, ymax, 0)
    pts.Modified()
    # because insertions don't modify object - performance reasons
    renWin.Render()

def MoveWindow ():
    y = 100
    while y < 300:
        x = 100
        while x < 300:
            PlaceWindow(x, y)
            x += 25
        y += 25


MoveWindow()
PlaceWindow(xmin, ymin)

#iren.Start()
