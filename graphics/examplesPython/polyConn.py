#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

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

# planes to connect
plane1 = vtkStructuredGridGeometryFilter()
plane1.SetInput(pl3d.GetOutput())
plane1.SetExtent(10,10,0,100,0,100)
conn = vtkPolyDataConnectivityFilter()
conn.SetInput(plane1.GetOutput())
conn.ScalarConnectivityOn()
scalarRange=(1.5,4.0)
conn.SetScalarRange(scalarRange)
plane1Map = vtkPolyDataMapper()
plane1Map.SetInput(conn.GetOutput())
plane1Map.SetScalarRange(pl3d.GetOutput().GetScalarRange())
plane1Actor = vtkActor()
plane1Actor.SetMapper(plane1Map)
plane1Actor.GetProperty().SetOpacity(0.999)

# outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(0,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(wallActor)
ren.AddActor(finActor)
ren.AddActor(plane1Actor)
ren.SetBackground(1,1,1)
renWin.SetSize(400,400)

cam1 = vtkCamera()
cam1.SetClippingRange(1.51176,75.5879)
cam1.SetFocalPoint(2.33749,2.96739,3.61023)
cam1.SetPosition(10.8787,5.27346,15.8687)
cam1.SetViewAngle(30)
cam1.SetViewUp(-0.0610856,0.987798,-0.143262)
ren.SetActiveCamera(cam1)

iren.Initialize()

renWin.SetFileName("polyConn.ppm")
iren.Start()
