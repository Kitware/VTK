#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# generate four hyperstreamlines

from vtkInclude import *

# create tensor ellipsoids

# Create the RenderWindow, Renderer and interactive renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

#
# Create tensor ellipsoids
#
# generate tensors
ptLoad = vtkPointLoad()
ptLoad.SetLoadValue(100.0)
ptLoad.SetSampleDimensions(30,30,30)
ptLoad.ComputeEffectiveStressOn()
ptLoad.SetModelBounds(-10,10,-10,10,-10,10)

# Generate hyperstreamlines
s1 = vtkHyperStreamline()
s1.SetInput(ptLoad.GetOutput())
s1.SetStartPosition(9,9,-9)
s1.IntegrateMinorEigenvector()
s1.SetMaximumPropagationDistance(18.0)
s1.SetIntegrationStepLength(0.1)
s1.SetStepLength(0.01)
s1.SetRadius(0.25)
s1.SetNumberOfSides(18)
s1.SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS)
s1.Update()
# Map hyperstreamlines
lut = vtkLogLookupTable()
lut.SetHueRange(.6667,0.0)
s1Mapper = vtkPolyDataMapper()
s1Mapper.SetInput(s1.GetOutput())
s1Mapper.SetLookupTable(lut)
ptLoad.Update()
#force update for scalar range
s1Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())
s1Actor = vtkActor()
s1Actor.SetMapper(s1Mapper)

s2 = vtkHyperStreamline()
s2.SetInput(ptLoad.GetOutput())
s2.SetStartPosition(-9,-9,-9)
s2.IntegrateMinorEigenvector()
s2.SetMaximumPropagationDistance(18.0)
s2.SetIntegrationStepLength(0.1)
s2.SetStepLength(0.01)
s2.SetRadius(0.25)
s2.SetNumberOfSides(18)
s2.SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS)
s2.Update()
s2Mapper = vtkPolyDataMapper()
s2Mapper.SetInput(s2.GetOutput())
s2Mapper.SetLookupTable(lut)
ptLoad.Update()  #force.update.for.scalar.range()

s2Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())
s2Actor = vtkActor()
s2Actor.SetMapper(s2Mapper)

s3 = vtkHyperStreamline()
s3.SetInput(ptLoad.GetOutput())
s3.SetStartPosition(9,-9,-9)
s3.IntegrateMinorEigenvector()
s3.SetMaximumPropagationDistance(18.0)
s3.SetIntegrationStepLength(0.1)
s3.SetStepLength(0.01)
s3.SetRadius(0.25)
s3.SetNumberOfSides(18)
s3.SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS)
s3.Update()
s3Mapper = vtkPolyDataMapper()
s3Mapper.SetInput(s3.GetOutput())
s3Mapper.SetLookupTable(lut)
ptLoad.Update() #force.update.for.scalar.range()

s3Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())
s3Actor = vtkActor()
s3Actor.SetMapper(s3Mapper)

s4 = vtkHyperStreamline()
s4.SetInput(ptLoad.GetOutput())
s4.SetStartPosition(-9,9,-9)
s4.IntegrateMinorEigenvector()
s4.SetMaximumPropagationDistance(18.0)
s4.SetIntegrationStepLength(0.1)
s4.SetStepLength(0.01)
s4.SetRadius(0.25)
s4.SetNumberOfSides(18)
s4.SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS)
s4.Update()
s4Mapper = vtkPolyDataMapper()
s4Mapper.SetInput(s4.GetOutput())
s4Mapper.SetLookupTable(lut)
ptLoad.Update() #force.update.for.scalar.range()
s4Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())
s4Actor = vtkActor()
s4Actor.SetMapper(s4Mapper)

#
# plane for context
#
g = vtkStructuredPointsGeometryFilter()
g.SetInput(ptLoad.GetOutput())
g.SetExtent(0,100,0,100,0,0)
g.Update() #for.scalar.range()
gm = vtkPolyDataMapper()
gm.SetInput(g.GetOutput())
gm.SetScalarRange(g.GetOutput().GetScalarRange())
ga = vtkActor()
ga.SetMapper(gm)

#
# Create outline around data
#
outline = vtkOutlineFilter()
outline.SetInput(ptLoad.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
#
# Create cone indicating application of load
#
coneSrc = vtkConeSource()
coneSrc.SetRadius(.5)
coneSrc.SetHeight(2)
coneMap = vtkPolyDataMapper()
coneMap.SetInput(coneSrc.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMap)
coneActor.SetPosition(0,0,11)
coneActor.RotateY(90)
coneActor.GetProperty().SetColor(1,0,0)

camera = vtkCamera()
camera.SetFocalPoint(0.113766,-1.13665,-1.01919)
camera.SetPosition(-29.4886,-63.1488,26.5807)
camera.SetViewAngle(24.4617)
camera.SetViewUp(0.17138,0.331163,0.927879)
camera.SetClippingRange(1,100)

ren.AddActor(s1Actor)
ren.AddActor(s2Actor)
ren.AddActor(s3Actor)
ren.AddActor(s4Actor)
ren.AddActor(outlineActor)
ren.AddActor(coneActor)
ren.AddActor(ga)
ren.SetBackground(1.0,1.0,1.0)
ren.SetActiveCamera(camera)

renWin.SetSize(500,500)
renWin.Render()

iren.Start()
