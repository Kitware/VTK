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

# create pipeline
#
pl3d2 = vtkPLOT3DReader()
pl3d2.SetXYZFileName(VTK_DATA + "/combxyz.bin")
pl3d2.SetQFileName(VTK_DATA + "/combq.bin")
pl3d2.SetScalarFunctionNumber(153)
pl3d2.Update()

pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA + "/combxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
iso = vtkContourFilter()
iso.SetInput(pl3d.GetOutput())
iso.SetValue(0,.24)

probe2 = vtkProbeFilter()
probe2.SetInput(iso.GetOutput())
probe2.SetSource(pl3d2.GetOutput())

cast2 = vtkCastToConcrete()
cast2.SetInput(probe2.GetOutput())

normals = vtkPolyDataNormals()
normals.SetMaxRecursionDepth(100)
normals.SetInput(cast2.GetPolyDataOutput())
normals.SetFeatureAngle(45)
isoMapper = vtkPolyDataMapper()
isoMapper.SetInput(normals.GetOutput())
isoMapper.ScalarVisibilityOn()
isoMapper.SetScalarRange(0,1500)

isoActor = vtkLODActor()
isoActor.SetMapper(isoMapper)
isoActor.SetNumberOfCloudPoints(1000)
isoActor.GetProperty().SetColor(bisque[0],bisque[1],bisque[2])

outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(isoActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
ren.SetBackground(0.1,0.2,0.4)

cam1=ren.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(9.71821,0.458166,29.3999)
cam1.SetPosition(2.7439,-37.3196,38.7167)
cam1.SetViewUp(-0.16123,0.264271,0.950876)

# render the image
#

renWin.Render()



iren.Start()
