#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *


# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA + "/combxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/combq.bin")
pl3d.SetScalarFunctionNumber(110)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

probeLine = vtkLineSource()
probeLine.SetPoint1(1,1,29)
probeLine.SetPoint2(16.5,5,31.7693)
probeLine.SetResolution(500)

probe = vtkProbeFilter()
probe.SetInput(probeLine.GetOutput())
probe.SetSource(pl3d.GetOutput())

probeTube = vtkTubeFilter()
probeTube.SetInput(probe.GetPolyDataOutput())
probeTube.SetNumberOfSides(5)
probeTube.SetRadius(.05)

probeMapper = vtkPolyDataMapper()
probeMapper.SetInput(probeTube.GetOutput())
probeMapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())

probeActor = vtkActor()
probeActor.SetMapper(probeMapper)

displayLine = vtkLineSource()
displayLine.SetPoint1(0,0,0)
displayLine.SetPoint2(1,0,0)
displayLine.SetResolution(probeLine.GetResolution())

displayMerge = vtkMergeFilter()
displayMerge.SetGeometry(displayLine.GetOutput())
displayMerge.SetScalars(probe.GetPolyDataOutput())

displayWarp = vtkWarpScalar()
displayWarp.SetInput(displayMerge.GetPolyDataOutput())
displayWarp.SetNormal(0,1,0)
displayWarp.SetScaleFactor(.000001)

displayMapper = vtkPolyDataMapper()
displayMapper.SetInput(displayWarp.GetPolyDataOutput())
displayMapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())

displayActor = vtkActor()
displayActor.SetMapper(displayMapper)

outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

ren.AddActor(outlineActor)
ren.AddActor(probeActor)
ren.SetBackground(1,1,1)
ren.SetViewport(0,.25,1,1)

ren2.AddActor(displayActor)
ren2.SetBackground(0,0,0)
ren2.SetViewport(0,0,1,.25)

renWin.SetSize(500,500)

cam1=ren.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(9.9,-26,41)
cam1.SetViewUp(0.060772,-0.319905,0.945498)

cam2=ren2.GetActiveCamera()
cam2.ParallelProjectionOn()
cam2.SetParallelScale(.15)

iren.Initialize()


# render the image
#




iren.Start()
