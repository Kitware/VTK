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

# planes to threshold
plane1 = vtkStructuredGridGeometryFilter()
plane1.SetInput(pl3d.GetOutput())
plane1.SetExtent(10,10,0,100,0,100)

#plane1Map = vtkPolyDataMapper()
plane1Map = vtkDataSetMapper()
plane1Map.SetInput(plane1.GetOutput())
pl3dPtData=pl3d.GetOutput().GetPointData()
pl3dScalars=pl3dPtData.GetScalars()
range1=pl3dScalars.GetRange()
print 'range1= ',range1
#range1=  (0.192599996924, 4.97749996185)
#plane1Map.SetScalarRange(pl3dScalars.GetRange())
plane1Map.SetScalarRange(range1)
plane1Actor = vtkActor()
plane1Actor.SetMapper(plane1Map)

plane2 = vtkStructuredGridGeometryFilter()
plane2.SetInput(pl3d.GetOutput())
plane2.SetExtent(25,25,0,100,0,100)

#plane2Map = vtkPolyDataMapper()
plane2Map = vtkDataSetMapper()
#rwh
plane2Map.ImmediateModeRenderingOn()
plane2Map.SetInput(plane2.GetOutput())
#plane2Map.SetScalarRange(pl3d.GetOutput().GetPointData().GetScalars().GetRange() )
plane2Map.SetScalarRange(range1)


plane2Actor = vtkActor()
plane2Actor.SetMapper(plane2Map)

plane3 = vtkStructuredGridGeometryFilter()
plane3.SetInput(pl3d.GetOutput())
plane3.SetExtent(35,35,0,100,0,100)

plane3Map = vtkDataSetMapper()
#rwh
plane3Map.ImmediateModeRenderingOn()
plane3Map.SetInput(plane3.GetOutput())
#plane3Map.SetScalarRange(pl3d.GetOutput().GetPointData().GetScalars().GetRange() )
plane3Map.SetScalarRange(range1)

plane3Actor = vtkActor()
plane3Actor.SetMapper(plane3Map)

# outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
#outlineMapper = vtkPolyDataMapper()
outlineMapper = vtkDataSetMapper()
#rwh
outlineMapper.ImmediateModeRenderingOn()
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
ren.AddActor(plane2Actor)
ren.AddActor(plane3Actor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

cam1=ren.GetActiveCamera()
cam1.Azimuth(-40)
cam1.Zoom(1.4)

iren.Initialize()
renWin.Render()

# render the image
#


iren.Start()
