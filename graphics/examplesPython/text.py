#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Created oriented text

# pipeline

text0Source = vtkTextSource()
text0Source.SetText("Text.Source.with.Scalars.(default)")

text0Mapper = vtkPolyDataMapper()
text0Mapper.SetInput(text0Source.GetOutput())

text0Actor = vtkActor()
text0Actor.SetMapper(text0Mapper)
text0Actor.SetScale(.1,.1,.1)
text0Actor.AddPosition(0,2,0)

text1Source = vtkTextSource()
text1Source.SetText("Text.Source.with.Scalars")
text1Source.SetForegroundColor(1,0,0)
text1Source.SetBackgroundColor(1,1,1)

text1Mapper = vtkPolyDataMapper()
text1Mapper.SetInput(text1Source.GetOutput())

text1Actor = vtkActor()
text1Actor.SetMapper(text1Mapper)
text1Actor.SetScale(.1,.1,.1)

text2Source = vtkTextSource()
text2Source.SetText("Text.Source.without.Scalars")
text2Source.BackingOff()

text2Mapper = vtkPolyDataMapper()
text2Mapper.SetInput(text2Source.GetOutput())
text2Mapper.ScalarVisibilityOff()

text2Actor = vtkActor()
text2Actor.SetMapper(text2Mapper)
text2Actor.GetProperty().SetColor(1,1,0)
text2Actor.SetScale(.1,.1,.1)
text2Actor.AddPosition(0,-2,0)

text3Source = vtkVectorText()
text3Source.SetText("Vector.Text")

text3Mapper = vtkPolyDataMapper()
text3Mapper.SetInput(text3Source.GetOutput())
text3Mapper.ScalarVisibilityOff()

text3Actor = vtkActor()
text3Actor.SetMapper(text3Mapper)
text3Actor.GetProperty().SetColor(.1,1,0)
text3Actor.AddPosition(0,-4,0)

# create graphics stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(500,500)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(text0Actor)
ren.AddActor(text1Actor)
ren.AddActor(text2Actor)
ren.AddActor(text3Actor)
ren.GetActiveCamera().Zoom(1.5)
ren.SetBackground(.1,.2,.4)

renWin.Render()

iren.Initialize()


iren.Start()
