#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# create selected streamlines in arteries
from colors import *
from vtkInclude import *

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
reader = vtkStructuredPointsReader()
reader.SetFileName(VTK_DATA + "/carotid.vtk")
psource = vtkPointSource()
psource.SetNumberOfPoints(25)
psource.SetCenter(133.1,116.3,5.0)
psource.SetRadius(2.0)
threshold = vtkThresholdPoints()
threshold.SetInput(reader.GetOutput())
threshold.ThresholdByUpper(275)
streamers = vtkStreamLine()
streamers.SetInput(reader.GetOutput())
streamers.SetSource(psource.GetOutput())
streamers.SetMaximumPropagationTime(100.0)
#streamers.SetIntegrationStepLength(0.2)
streamers.SetIntegrationStepLength(0.1)
streamers.SpeedScalarsOn()
streamers.SetTerminalSpeed(.1)
tubes = vtkTubeFilter()
tubes.SetInput(streamers.GetOutput())
tubes.SetRadius(0.3)
tubes.SetNumberOfSides(6)
tubes.SetVaryRadius(VTK_VARY_RADIUS_OFF)
lut = vtkLookupTable()
lut.SetHueRange(.667,0.0)
lut.Build()
streamerMapper = vtkPolyDataMapper()
#rwh- add
streamerMapper.ImmediateModeRenderingOn()
streamerMapper.SetInput(tubes.GetOutput())
streamerMapper.SetScalarRange(2,10)
streamerMapper.SetLookupTable(lut)
streamerActor = vtkActor()
streamerActor.SetMapper(streamerMapper)

# contours of speed
iso = vtkContourFilter()
iso.SetInput(reader.GetOutput())
iso.SetValue(0,190)
isoMapper = vtkPolyDataMapper()
#rwh- add
isoMapper.ImmediateModeRenderingOn()
isoMapper.SetInput(iso.GetOutput())
isoMapper.ScalarVisibilityOff()
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetRepresentationToWireframe()
isoActor.GetProperty().SetOpacity(0.25)

# outline
outline = vtkOutlineFilter()
outline.SetInput(reader.GetOutput())
outlineMapper = vtkPolyDataMapper()
#rwh- add
outlineMapper.ImmediateModeRenderingOn()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(0,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(streamerActor)
ren.AddActor(isoActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

cam1 = vtkCamera()
cam1.SetClippingRange(17.4043,870.216)
cam1.SetFocalPoint(136.71,104.025,23)
cam1.SetPosition(204.747,258.939,63.7925)
cam1.SetViewUp(-0.102647,-0.210897,0.972104)
cam1.Zoom(1.6)
ren.SetActiveCamera(cam1)

iren.Initialize()

# render the image
#
#commandloop."puts(-nonewline,vtki>".puts.cont)

renWin.Render()

iren.Start()
