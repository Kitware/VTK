#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#
# create a triangular texture on a sphere
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400,400)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)


aTriangularTexture = vtkTriangularTexture()
aTriangularTexture.SetTexturePattern(2)
aTriangularTexture.SetScaleFactor(1.3)
aTriangularTexture.SetXSize(64)
aTriangularTexture.SetYSize(64)
  
aSphere = vtkSphereSource()
aSphere.SetThetaResolution(20)
aSphere.SetPhiResolution(20)

tCoords = vtkTriangularTCoords()
tCoords.SetInput(aSphere.GetOutput())

triangleMapper = vtkPolyDataMapper()
triangleMapper.SetInput(tCoords.GetOutput())

aTexture = vtkTexture()
aTexture.SetInput(aTriangularTexture.GetOutput())
aTexture.InterpolateOn()

banana=(0.8900, 0.8100, 0.3400)
texturedActor = vtkActor()
texturedActor.SetMapper(triangleMapper)
texturedActor.SetTexture(aTexture)
texturedActor.GetProperty().BackfaceCullingOn()
#texturedActor.GetProperty().SetDiffuseColor(banana[0],banana[1],banana[2])
texturedActor.GetProperty().SetDiffuseColor(banana)
texturedActor.GetProperty().SetSpecular(.4)
texturedActor.GetProperty().SetSpecularPower(40)

aCube = vtkCubeSource()
aCube.SetXLength(.5)
aCube.SetYLength(.5)

aCubeMapper = vtkPolyDataMapper()
aCubeMapper.SetInput(aCube.GetOutput())

tomato=(1.0000,0.3882,0.2784)
cubeActor = vtkActor()
cubeActor.SetMapper(aCubeMapper)
cubeActor.GetProperty().SetDiffuseColor(tomato)

slate_grey=(0.4392,0.5020,0.5647)
ren.SetBackground(slate_grey)
ren.AddActor(cubeActor)
ren.AddActor(texturedActor)
ren.GetActiveCamera().Zoom(1.5)

# render the image
#
iren.Initialize()
renWin.SetFileName("triangularTCoords.ppm")

iren.Start()
