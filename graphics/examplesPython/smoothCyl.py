#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

from colors import *

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a semi-cylinder
#
line = vtkLineSource()
line.SetPoint1(0,1,0)
line.SetPoint2(0,1,2)
line.SetResolution(10)

lineSweeper = vtkRotationalExtrusionFilter()
lineSweeper.SetResolution(20)
lineSweeper.SetInput(line.GetOutput())
lineSweeper.SetAngle(270)

bump = vtkBrownianPoints()
bump.SetInput(lineSweeper.GetOutput())

warp = vtkWarpVector()
warp.SetInput(bump.GetPolyDataOutput())
warp.SetScaleFactor(.2)

smooth = vtkSmoothPolyDataFilter()
smooth.SetInput(warp.GetPolyDataOutput())
smooth.SetNumberOfIterations(50)
smooth.BoundarySmoothingOn()
smooth.SetFeatureAngle(120)
smooth.SetEdgeAngle(90)
smooth.SetRelaxationFactor(.025)

normals = vtkPolyDataNormals()
normals.SetInput(smooth.GetOutput())

cylMapper = vtkPolyDataMapper()
cylMapper.SetInput(normals.GetOutput())

cylActor = vtkActor()
cylActor.SetMapper(cylMapper)
cylActor.GetProperty().SetInterpolationToGouraud()
cylActor.GetProperty().SetInterpolationToFlat()
cylActor.GetProperty().SetColor(beige[0],beige[1],beige[2])

originalMapper = vtkPolyDataMapper()
originalMapper.SetInput(bump.GetPolyDataOutput())

originalActor = vtkActor()
originalActor.SetMapper(originalMapper)
originalActor.GetProperty().SetInterpolationToFlat()
cylActor.GetProperty().SetColor(tomato[0],tomato[1],tomato[2])

# Add the actors to the renderer, set the background and size
#
ren.AddActor(cylActor)
#ren1 AddActor originalActor
ren.SetBackground(1,1,1)
renWin.SetSize(350,450)

camera = vtkCamera()
camera.SetClippingRange(0.576398,28.8199)
camera.SetFocalPoint(0.0463079,-0.0356571,1.01993)
camera.SetPosition(-2.47044,2.39516,-3.56066)
camera.SetViewUp(0.607296,-0.513537,-0.606195)
ren.SetActiveCamera(camera)

iren.Initialize()
renWin.SetFileName("valid/smoothCyl.ppm")

iren.Start()
