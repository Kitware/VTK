#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

reader = vtkSTLReader()
reader.SetFileName(VTK_DATA + "/42400-IDGH.stl")
dataMapper = vtkPolyDataMapper()
dataMapper.SetInput(reader.GetOutput())
model = vtkActor()
model.SetMapper(dataMapper)
model.GetProperty().SetColor(1,0,0)

obb = vtkOBBTree()
obb.SetMaxLevel(4)
obb.SetNumberOfCellsPerBucket(4)
boxes = vtkSpatialRepresentationFilter()
boxes.SetInput(reader.GetOutput())
boxes.SetSpatialRepresentation(obb)
boxMapper = vtkPolyDataMapper()
boxMapper.SetInput(boxes.GetOutput())
boxActor = vtkActor()
boxActor.SetMapper(boxMapper)
boxActor.GetProperty().SetAmbient(1)
boxActor.GetProperty().SetDiffuse(0)
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
ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()

iren.Start()
