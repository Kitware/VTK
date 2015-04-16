#!/usr/bin/env python

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

current_font_size = 16

default_text = "ABCDEFGHIJKLMnopqrstuvwxyz"

text_color = (246/255.0, 255/255.0, 11/255.0)

bg_color = (56/255.0, 56/255.0, 154/255.0)

renWin = vtk.vtkRenderWindow()
renWin.SetSize(386, 120)

ren = vtk.vtkRenderer()
ren.SetBackground(bg_color)
renWin.AddRenderer(ren)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

i = 0

# another actor to test occlusion
plane = vtk.vtkPlaneSource()
plane.SetOrigin(120, 10, 0)
plane.SetPoint1(220, 10, 0)
plane.SetPoint2(120, 110, 0)

mapper = vtk.vtkPolyDataMapper2D()
mapper.SetInputConnection(plane.GetOutputPort())

actor = vtk.vtkActor2D()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0.5, 0.5, 0.5)

ren.AddActor(actor)

for (bold,italic,shadow) in ((0,0,0),(0,0,1),(1,0,0),(0,1,0),(1,1,0)):
    i = i + 1
    attribs = []
    if bold:
        attribs.append("b")

    if italic:
        attribs.append("i")

    if shadow:
        attribs.append("s")

    face_name = "Arial"
    if attribs:
        face_name = face_name + " (" + ",".join(attribs) + ")"

    mapper = vtk.vtkTextMapper()
    mapper.SetInput(face_name + ": " + default_text)

    tprop = mapper.GetTextProperty()
    tprop.SetFontFamilyToArial()
    tprop.SetColor(text_color[0],text_color[1],text_color[2])
    tprop.SetBold(bold)
    tprop.SetItalic(italic)
    tprop.SetShadow(shadow)
    tprop.SetFontSize(current_font_size)

    actor = vtk.vtkActor2D()
    actor.SetMapper(mapper)
    actor.SetDisplayPosition(10, i*(current_font_size+5))

    ren.AddActor(actor)

iren.Initialize()
renWin.Render()
#iren.Start()
