#!/usr/local/bin/python
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
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA + "/combxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
plane = vtkStructuredGridGeometryFilter()
plane.SetInput(pl3d.GetOutput())
plane.SetExtent(10,10,1,100,1,100)
plane2 = vtkStructuredGridGeometryFilter()
plane2.SetInput(pl3d.GetOutput())
plane2.SetExtent(30,30,1,100,1,100)
plane3 = vtkStructuredGridGeometryFilter()
plane3.SetInput(pl3d.GetOutput())
plane3.SetExtent(45,45,1,100,1,100)
appendF = vtkAppendPolyData()
appendF.AddInput(plane.GetOutput())
appendF.AddInput(plane2.GetOutput())
appendF.AddInput(plane3.GetOutput())
warp = vtkWarpScalar()
warp.SetInput(appendF.GetOutput())
warp.UseNormalOn()
warp.SetNormal(1.0,0.0,0.0)
warp.SetScaleFactor(2.5)
normals = vtkPolyDataNormals()
normals.SetInput(warp.GetPolyDataOutput())
normals.SetFeatureAngle(60)
planeMapper = vtkPolyDataMapper()
planeMapper.SetInput(normals.GetOutput())
planeMapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)

outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

cam1=ren.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)
iren.Initialize()

# render the image
#
renWin.Render()





iren.Start()
