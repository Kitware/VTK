#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Create dashed streamlines

from colors import *
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read data
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA + "/bluntfinxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/bluntfinq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

#streamers
#
seeds = vtkLineSource()
seeds.SetResolution(25)
seeds.SetPoint1(-6.5,0.25,0.10)
seeds.SetPoint2(-6.5,0.25,5.0)
streamers = vtkDashedStreamLine()
streamers.SetInput(pl3d.GetOutput())
streamers.SetSource(seeds.GetOutput())
streamers.SetMaximumPropagationTime(25)
streamers.SetStepLength(0.25)
streamers.Update()
mapStreamers = vtkPolyDataMapper()
mapStreamers.SetInput(streamers.GetOutput())
mapStreamers.SetScalarRange(  \
pl3d.GetOutput().GetPointData().GetScalars().GetRange() )
streamersActor = vtkActor()
streamersActor.SetMapper(mapStreamers)

# wall
#
wall = vtkStructuredGridGeometryFilter()
wall.SetInput(pl3d.GetOutput())
wall.SetExtent(0,100,0,0,0,100)
wallMap = vtkPolyDataMapper()
wallMap.SetInput(wall.GetOutput())
wallMap.ScalarVisibilityOff()
wallActor = vtkActor()
wallActor.SetMapper(wallMap)
wallActor.GetProperty().SetColor(0.8,0.8,0.8)

# fin
# 
fin = vtkStructuredGridGeometryFilter()
fin.SetInput(pl3d.GetOutput())
fin.SetExtent(0,100,0,100,0,0)
finMap = vtkPolyDataMapper()
finMap.SetInput(fin.GetOutput())
finMap.ScalarVisibilityOff()
finActor = vtkActor()
finActor.SetMapper(finMap)
finActor.GetProperty().SetColor(0.8,0.8,0.8)

# outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(1,1,1)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(streamersActor)
ren.AddActor(wallActor)
ren.AddActor(finActor)
ren.SetBackground(0,0,0)
renWin.SetSize(700,500)

cam1 = vtkCamera()
cam1.SetFocalPoint(2.87956,4.24691,2.73135)
cam1.SetPosition(-3.46307,16.7005,29.7406)
cam1.SetViewAngle(30)
cam1.SetViewUp(0.127555,0.911749,-0.390441)
ren.SetActiveCamera(cam1)

iren.Initialize()
renWin.Render()

# render the image
#

renWin.Render()


iren.Start()
