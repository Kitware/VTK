#!/usr/bin/env python

from vtkmodules.vtkCommonCore import (
    reference,
    vtkPoints,
    vtkUnsignedCharArray,
)

from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkCellData,
    vtkPolyData,
)

from vtkmodules.vtkRenderingCore import (
    vtkActor2D,
    vtkPolyDataMapper2D,
    vtkProperty2D,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkTextActor,
    vtkTextProperty,
)

from vtkmodules.vtkRenderingFreeType import (
    vtkMathTextUtilities,
)

import vtkmodules.vtkRenderingMatplotlib
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkRenderingUI

def setupTextActor(actor, anchor):
    p = actor.GetTextProperty()
    label = (str(p.GetVerticalJustificationAsString()[0]) +
             str(p.GetJustificationAsString()[0]) + " " +
             "$\\theta = " + str(round(p.GetOrientation())) + "$")
    actor.SetInput(label)

    # Add the anchor point:
    pos = actor.GetPosition()
    col = p.GetColor()
    ptId = reference((anchor.GetPoints().InsertNextPoint(pos[0], pos[1], 0.),))
    anchor.GetVerts().InsertNextCell(1, ptId)
    anchor.GetCellData().GetScalars().InsertNextTuple4(
        col[0] * 255, col[1] * 255, col[2] * 255, 255)

ren = vtkRenderer()

width = 600
height = 600
x = ( 100, 300, 500 )
y = ( 100, 300, 500 )

# Render the anchor points to check alignment:
anchors = vtkPolyData()
points = vtkPoints()
anchors.SetPoints(points)
verts = vtkCellArray()
anchors.SetVerts(verts)
colors = vtkUnsignedCharArray()
colors.SetNumberOfComponents(4)
anchors.GetCellData().SetScalars(colors)

for row in range(3):
    for col in range(3):
        actor = vtkTextActor()
        if row == 0:
            actor.GetTextProperty().SetJustificationToRight()
        elif row == 1:
            actor.GetTextProperty().SetJustificationToCentered()
        elif row == 2:
            actor.GetTextProperty().SetJustificationToLeft()
        if col == 0:
            actor.GetTextProperty().SetVerticalJustificationToBottom()
        elif col == 1:
            actor.GetTextProperty().SetVerticalJustificationToCentered()
        elif col == 2:
            actor.GetTextProperty().SetVerticalJustificationToTop()
        actor.GetTextProperty().SetFontSize(22)
        actor.GetTextProperty().SetOrientation(45.0 * (3 * row + col))
        actor.GetTextProperty().SetColor(0.75, .2 + col * .26, .2 + row * .26)
        actor.GetTextProperty().SetBackgroundColor(0., 1. - col * .26, 1. - row * .26)
        actor.GetTextProperty().SetBackgroundOpacity(0.25)
        actor.GetTextProperty().SetFrame((row + col) % 9 == 0)
        actor.GetTextProperty().SetFrameColor(
            (0., 1.)[col > 0], (0., 1.)[col == 1], (0., 1.)[col < 2])
        actor.GetTextProperty().SetFrameWidth(1)
        actor.SetPosition(x[col], y[row])
        setupTextActor(actor, anchors)
        ren.AddActor(actor)

anchorMapper = vtkPolyDataMapper2D()
anchorMapper.SetInputData(anchors)
anchorActor = vtkActor2D()
anchorActor.SetMapper(anchorMapper)
anchorActor.GetProperty().SetPointSize(5)
ren.AddActor(anchorActor)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.SetBackground(0.0, 0.0, 0.0)
renWin.SetSize(width, height)

renWin.SetMultiSamples(0)
iren.Initialize()
if 'rtTester' not in locals() or rtTester.IsInteractiveModeSpecified():
    iren.Start()
