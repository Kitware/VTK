#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

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
ptLoad.SetSampleDimensions(6,6,6)
ptLoad.ComputeEffectiveStressOn()
ptLoad.SetModelBounds(-10,10,-10,10,-10,10)

# extract plane of data
plane = vtkStructuredPointsGeometryFilter()
plane.SetInput(ptLoad.GetOutput())
plane.SetExtent(2,2,0,99,0,99)

# Generate ellipsoids
sphere = vtkSphereSource()
sphere.SetThetaResolution(8)
sphere.SetPhiResolution(8)
ellipsoids = vtkTensorGlyph()
ellipsoids.SetInput(ptLoad.GetOutput())
ellipsoids.SetSource(sphere.GetOutput())
ellipsoids.SetScaleFactor(10)
ellipsoids.ClampScalingOn()
  
ellipNormals = vtkPolyDataNormals()
ellipNormals.SetInput(ellipsoids.GetOutput())

# Map contour
lut = vtkLogLookupTable()
lut.SetHueRange(.6667,0.0)
ellipMapper = vtkPolyDataMapper()
ellipMapper.SetInput(ellipNormals.GetOutput())
ellipMapper.SetLookupTable(lut)
plane.Update() #force.update.for.scalar.range()
ellipMapper.SetScalarRange(plane.GetOutput().GetScalarRange())

ellipActor = vtkActor()
ellipActor.SetMapper(ellipMapper)
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

ren.AddActor(ellipActor)
ren.AddActor(outlineActor)
ren.AddActor(coneActor)
ren.SetBackground(1.0,1.0,1.0)
ren.SetActiveCamera(camera)

renWin.SetSize(450,450)
renWin.Render()

iren.Start()
