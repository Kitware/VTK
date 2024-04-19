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
    vtkActor,
    vtkCamera,
    vtkPolyDataMapper,
    vtkProperty,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkTextActor3D,
    vtkTextProperty,
)

from vtkmodules.vtkRenderingFreeType import (
    vtkMathTextUtilities,
)

import vtkmodules.vtkRenderingMatplotlib
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkRenderingUI

def setupTextActor3D(actor, anchor):
    p = actor.GetTextProperty()
    label = (str(p.GetVerticalJustificationAsString()[0]) +
             str(p.GetJustificationAsString()[0]) + " " +
             "$\\theta = " + str(round(p.GetOrientation())) + "$")
    actor.SetInput(label)

    # Add the anchor point:
    pos = actor.GetPosition()
    col = p.GetColor()
    ptId = reference((anchor.GetPoints().InsertNextPoint(pos[0], pos[1], pos[2]),))
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
        actor = vtkTextActor3D()
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
        actor.GetTextProperty().SetFontSize(20)
        actor.GetTextProperty().SetOrientation(45.0 * (3 * row + col))
        actor.GetTextProperty().SetColor(0.75, .2 + col * .26, .2 + row * .26)
        actor.GetTextProperty().SetBackgroundColor(0., 1. - col * .26, 1. - row * .26)
        actor.GetTextProperty().SetBackgroundOpacity(0.25)
        actor.SetPosition(x[col], y[row], 0.)
        setupTextActor3D(actor, anchors)
        ren.AddActor(actor)

anchorMapper = vtkPolyDataMapper()
anchorMapper.SetInputData(anchors)
anchorActor = vtkActor()
anchorActor.SetMapper(anchorMapper)
anchorActor.GetProperty().SetPointSize(5)
ren.AddActor(anchorActor)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.SetBackground(0.0, 0.0, 0.0)
ren.GetActiveCamera().SetPosition(width // 2, height // 2, 1400)
ren.GetActiveCamera().SetFocalPoint(width // 2, height // 2, 0)
ren.GetActiveCamera().SetViewUp(0, 1, 0)
ren.ResetCameraClippingRange()
renWin.SetSize(width, height)

renWin.SetMultiSamples(0)
iren.Initialize()
if 'rtTester' not in locals() or rtTester.IsInteractiveModeSpecified():
    iren.Start()
