#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *


# create planes
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

plane = vtkPlaneSource()
plane.SetResolution(50,50)
transP1 = vtkTransform()
transP1.Translate(3.7,0.0,28.37)
transP1.Scale(5,5,5)
transP1.RotateY(90)
tpd1 = vtkTransformPolyDataFilter()
tpd1.SetInput(plane.GetOutput())
tpd1.SetTransform(transP1)
outTpd1 = vtkOutlineFilter()
outTpd1.SetInput(tpd1.GetOutput())
mapTpd1 = vtkPolyDataMapper()
mapTpd1.SetInput(outTpd1.GetOutput())
tpd1Actor = vtkActor()
tpd1Actor.SetMapper(mapTpd1)
tpd1Actor.GetProperty().SetColor(0,0,0)

transP2 = vtkTransform()
transP2.Translate(9.2,0.0,31.20)
transP2.Scale(5,5,5)
transP2.RotateY(90)
tpd2 = vtkTransformPolyDataFilter()
tpd2.SetInput(plane.GetOutput())
tpd2.SetTransform(transP2)
outTpd2 = vtkOutlineFilter()
outTpd2.SetInput(tpd2.GetOutput())
mapTpd2 = vtkPolyDataMapper()
mapTpd2.SetInput(outTpd2.GetOutput())
tpd2Actor = vtkActor()
tpd2Actor.SetMapper(mapTpd2)
tpd2Actor.GetProperty().SetColor(0,0,0)

transP3 = vtkTransform()
transP3.Translate(13.27,0.0,33.30)
transP3.Scale(5,5,5)
transP3.RotateY(90)
tpd3 = vtkTransformPolyDataFilter()
tpd3.SetInput(plane.GetOutput())
tpd3.SetTransform(transP3)
outTpd3 = vtkOutlineFilter()
outTpd3.SetInput(tpd3.GetOutput())
mapTpd3 = vtkPolyDataMapper()
mapTpd3.SetInput(outTpd3.GetOutput())
tpd3Actor = vtkActor()
tpd3Actor.SetMapper(mapTpd3)
tpd3Actor.GetProperty().SetColor(0,0,0)

appendF = vtkAppendPolyData()
appendF.AddInput(tpd1.GetOutput())
appendF.AddInput(tpd2.GetOutput())
appendF.AddInput(tpd3.GetOutput())

probe = vtkProbeFilter()
probe.SetInput(appendF.GetOutput())
probe.SetSource(pl3d.GetOutput())

contour = vtkContourFilter()
contour.SetInput(probe.GetOutput())
# didn't get python-wrapped?
#contour.GenerateValues(50, pl3d.GetOutput().GetScalarRange() )
range = pl3d.GetOutput().GetScalarRange()
contour.GenerateValues(50, range[0],range[1])

contourMapper = vtkPolyDataMapper()
contourMapper.SetInput(contour.GetOutput())
contourMapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())
planeActor = vtkActor()
planeActor.SetMapper(contourMapper)

outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.AddActor(tpd1Actor)
ren.AddActor(tpd2Actor)
ren.AddActor(tpd3Actor)
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




iren.Start()
