#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

## Display a rectilinear grid and some common visualization techniques
##
from colors import *
from vtkInclude import *

# create pipeline
#
reader = vtkRectilinearGridReader()
reader.SetFileName(VTK_DATA + "/RectGrid.vtk")
reader.Update()
plane = vtkRectilinearGridGeometryFilter()
plane.SetInput(reader.GetOutput())
plane.SetExtent(0,100,0,100,15,15)
warper = vtkWarpVector()
warper.SetInput(plane.GetOutput())
warper.SetScaleFactor(0.05)
planeMapper = vtkDataSetMapper()
planeMapper.SetInput(warper.GetOutput())
planeMapper.SetScalarRange(0.197813,0.710419)
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)

cutPlane = vtkPlane()
cutPlane.SetOrigin(reader.GetOutput().GetCenter())
cutPlane.SetNormal(1,0,0)
planeCut = vtkCutter()
planeCut.SetInput(reader.GetOutput())
planeCut.SetCutFunction(cutPlane)
cutMapper = vtkDataSetMapper()
cutMapper.SetInput(planeCut.GetOutput())
cutMapper.SetScalarRange(  \
	reader.GetOutput().GetPointData().GetScalars().GetRange() )
cutActor = vtkActor()
cutActor.SetMapper(cutMapper)

iso = vtkContourFilter()
iso.SetInput(reader.GetOutput())
iso.SetValue(0,0.7)
normals = vtkPolyDataNormals()
normals.SetInput(iso.GetOutput())
normals.SetFeatureAngle(45)
isoMapper = vtkPolyDataMapper()
isoMapper.SetInput(normals.GetOutput())
isoMapper.ScalarVisibilityOff()
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(bisque[0],bisque[1],bisque[2])
isoActor.GetProperty().SetRepresentationToWireframe()

streamer = vtkStreamLine()
streamer.SetInput(reader.GetOutput())
streamer.SetStartPosition(-1.2,-0.1,1.3)
streamer.SetMaximumPropagationTime(500)
streamer.SetStepLength(0.05)
streamer.SetIntegrationStepLength(0.05)
streamer.SetIntegrationDirectionToIntegrateBothDirections()
streamer.Update()
streamTube = vtkTubeFilter()
streamTube.SetInput(streamer.GetOutput())
streamTube.SetRadius(0.025)
streamTube.SetNumberOfSides(6)
streamTube.SetVaryRadius(VTK_VARY_RADIUS_BY_VECTOR)
mapStreamTube = vtkPolyDataMapper()
mapStreamTube.SetInput(streamTube.GetOutput())
mapStreamTube.SetScalarRange(  \
	reader.GetOutput().GetPointData().GetScalars().GetRange() )
streamTubeActor = vtkActor()
streamTubeActor.SetMapper(mapStreamTube)
streamTubeActor.GetProperty().BackfaceCullingOn()

outline = vtkOutlineFilter()
outline.SetInput(reader.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(black[0],black[1],black[2])

# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.AddActor(cutActor)
ren.AddActor(isoActor)
ren.AddActor(streamTubeActor)

ren.SetBackground(1,1,1)
renWin.SetSize(400,400)

cam1=ren.GetActiveCamera()
cam1.SetClippingRange(1.04427,52.2137)
cam1.SetFocalPoint(0.106213,0.0196539,2.10569)
cam1.SetPosition(-7.34153,4.54201,7.86157)
cam1.SetViewUp(0.113046,0.847094,-0.519281)

iren.Initialize()

# render the image
#




iren.Start()
