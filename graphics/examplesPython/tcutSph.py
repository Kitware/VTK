#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#
# cut an outer sphere to reveal an inner sphere
#
# converted from tcutSph.cxx

from colors import *
# Create the RenderWindow  Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# hidden sphere
sphere1 = vtkSphereSource()
sphere1.SetThetaResolution(12)
sphere1.SetPhiResolution(12)
sphere1.SetRadius(0.5)

innerMapper = vtkPolyDataMapper()
innerMapper.SetInput(sphere1.GetOutput())

innerSphere = vtkActor()
innerSphere.SetMapper(innerMapper)
innerSphere.GetProperty().SetColor(1,.9216,.8039)

# sphere to texture
sphere2 = vtkSphereSource()
sphere2.SetThetaResolution(24)
sphere2.SetPhiResolution(24)
sphere2.SetRadius(1.0)

points = vtkPoints()
points.InsertPoint(0,0,0,0)
points.InsertPoint(1,0,0,0)

normals = vtkNormals()
normals.InsertNormal(0,1,0,0)
normals.InsertNormal(1,0,1,0)

planes = vtkPlanes()
planes.SetPoints(points)
planes.SetNormals(normals)

tcoords = vtkImplicitTextureCoords()
tcoords.SetInput(sphere2.GetOutput())
tcoords.SetRFunction(planes)

outerMapper = vtkDataSetMapper()
outerMapper.SetInput(tcoords.GetOutput())

tmap = vtkStructuredPointsReader()
tmap.SetFileName(VTK_DATA + "/texThres.vtk")

texture = vtkTexture()
texture.SetInput(tmap.GetOutput())
texture.InterpolateOff()
texture.RepeatOff()

outerSphere = vtkActor()
outerSphere.SetMapper(outerMapper)
outerSphere.SetTexture(texture)
outerSphere.GetProperty().SetColor(1,.6275,.4784)

ren.AddActor(innerSphere)
ren.AddActor(outerSphere)
ren.SetBackground(0.4392,0.5020,0.5647)
renWin.SetSize(500,500)

# interact with data
renWin.Render()
iren.Initialize()


iren.Start()
