#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# lines make a nice test
line1 = vtkLineSource()
line1.SetPoint1(0,0,0)
line1.SetPoint2(1,0,0)
line1.SetResolution(1000)
line2 = vtkLineSource()
line2.SetPoint1(0,0,0)
line2.SetPoint2(1,1,1)
line2.SetResolution(1000)
#vtkAppendPolyData asource
#  asource AddInput [line1 GetOutput]
#  asource AddInput [line2 GetOutput]

asource = vtkSTLReader()
asource.SetFileName(VTK_DATA + "/42400-IDGH.stl")
#vtkCyberReader asource
#  asource SetFileName ../../../vtkdata/fran_cut
dataMapper = vtkPolyDataMapper()
dataMapper.SetInput(asource.GetOutput())
model = vtkActor()
model.SetMapper(dataMapper)
model.GetProperty().SetColor(1,0,0)
#  model VisibilityOff

#vtkPointLocator locator
#vtkOBBTree locator
locator = vtkCellLocator()
locator.SetMaxLevel(4)
locator.AutomaticOff()
boxes = vtkSpatialRepresentationFilter()
boxes.SetInput(asource.GetOutput())
boxes.SetSpatialRepresentation(locator)
boxMapper = vtkPolyDataMapper()
boxMapper.SetInput(boxes.GetOutput())
#  boxMapper SetInput [boxes GetOutput 2]
boxActor = vtkActor()
boxActor.SetMapper(boxMapper)
boxActor.GetProperty().SetDiffuse(0)
boxActor.GetProperty().SetAmbient(1)
boxActor.GetProperty().SetRepresentationToWireframe()

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(model)
ren.AddActor(boxActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)
iren.Initialize()

ren.GetActiveCamera().Zoom(1.4)
renWin.Render()

iren.Start()
