#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

## LOx post CFD case study

#from colors import *
# read data
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA + "/postxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/postq.bin")
pl3d.SetScalarFunctionNumber(153)
pl3d.SetVectorFunctionNumber(200)
pl3d.Update()

#blue to red lut
#
lut = vtkLookupTable()
lut.SetHueRange(0.667,0.0)

# computational planes
floorComp = vtkStructuredGridGeometryFilter()
floorComp.SetExtent(0,37,0,75,0,0)
floorComp.SetInput(pl3d.GetOutput())
floorComp.Update()
floorMapper = vtkPolyDataMapper()
floorMapper.SetInput(floorComp.GetOutput())
floorMapper.ScalarVisibilityOff()
floorMapper.SetLookupTable(lut)
floorActor = vtkActor()
floorActor.SetMapper(floorMapper)
floorActor.GetProperty().SetRepresentationToWireframe()
floorActor.GetProperty().SetColor(0,0,0)

subFloorComp = vtkStructuredGridGeometryFilter()
subFloorComp.SetExtent(0,37,0,15,22,22)
subFloorComp.SetInput(pl3d.GetOutput())
subFloorMapper = vtkPolyDataMapper()
subFloorMapper.SetInput(subFloorComp.GetOutput())
subFloorMapper.SetLookupTable(lut)
subFloorMapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())
subFloorActor = vtkActor()
subFloorActor.SetMapper(subFloorMapper)

subFloor2Comp = vtkStructuredGridGeometryFilter()
subFloor2Comp.SetExtent(0,37,60,75,22,22)
subFloor2Comp.SetInput(pl3d.GetOutput())
subFloor2Mapper = vtkPolyDataMapper()
subFloor2Mapper.SetInput(subFloor2Comp.GetOutput())
subFloor2Mapper.SetLookupTable(lut)
subFloor2Mapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())
subFloor2Actor = vtkActor()
subFloor2Actor.SetMapper(subFloor2Mapper)

postComp = vtkStructuredGridGeometryFilter()
postComp.SetExtent(10,10,0,75,0,37)
postComp.SetInput(pl3d.GetOutput())
postMapper = vtkPolyDataMapper()
postMapper.SetInput(postComp.GetOutput())
postMapper.SetLookupTable(lut)
postMapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())
postActor = vtkActor()
postActor.SetMapper(postMapper)
postActor.GetProperty().SetColor(0,0,0)

fanComp = vtkStructuredGridGeometryFilter()
fanComp.SetExtent(0,37,38,38,0,37)
fanComp.SetInput(pl3d.GetOutput())
fanMapper = vtkPolyDataMapper()
fanMapper.SetInput(fanComp.GetOutput())
fanMapper.SetLookupTable(lut)
fanMapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())
fanActor = vtkActor()
fanActor.SetMapper(fanMapper)
fanActor.GetProperty().SetColor(0,0,0)

# streamers
#
# spherical seed points
rake = vtkPointSource()
rake.SetCenter(-0.74,0,0.3)
rake.SetNumberOfPoints(10)

# a line of seed points
seedsComp = vtkStructuredGridGeometryFilter()
seedsComp.SetExtent(10,10,37,39,1,35)
seedsComp.SetInput(pl3d.GetOutput())

streamers = vtkStreamLine()
streamers.SetInput(pl3d.GetOutput())
#streamers.SetSource(rake.GetOutput())
streamers.SetSource(seedsComp.GetOutput())
streamers.SetMaximumPropagationTime(250)
streamers.SpeedScalarsOn()
#streamers.SetIntegrationStepLength(.2)
streamers.SetIntegrationStepLength(.4)
#streamers.SetStepLength(.25)

tubes = vtkTubeFilter()
tubes.SetInput(streamers.GetOutput())
tubes.SetNumberOfSides(8)
#tubes.SetRadius(.08)
tubes.SetRadius(.06)
tubes.SetVaryRadius(0)
mapTubes = vtkPolyDataMapper()
mapTubes.SetInput(tubes.GetOutput())
mapTubes.SetScalarRange(pl3d.GetOutput().GetScalarRange())
tubesActor = vtkActor()
tubesActor.SetMapper(mapTubes)

# outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(0,0,0)

# Create graphics stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(floorActor)
ren.AddActor(subFloorActor)
ren.AddActor(subFloor2Actor)
ren.AddActor(postActor)
ren.AddActor(fanActor)
ren.AddActor(tubesActor)

aCam = vtkCamera()
aCam.SetFocalPoint(0.00657892,0,2.41026)
aCam.SetPosition(-1.94838,-47.1275,39.4607)
aCam.SetViewUp(0.00653193,0.617865,0.786257)

ren.SetBackground(.1,.2,.4)
ren.SetActiveCamera(aCam)
renWin.SetSize(256,256)

iren.Initialize()
renWin.Render()

iren.Start()
