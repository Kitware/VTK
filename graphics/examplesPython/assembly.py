#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# this demonstrates use of assemblies
# include get the vtk interactor ui

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create four parts: a top level assembly and three primitives
#
sphere = vtkSphereSource()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInput(sphere.GetOutput())
sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.SetOrigin(2,1,3)
sphereActor.RotateY(6)
sphereActor.SetPosition(2.25,0,0)
sphereActor.GetProperty().SetColor(1,0,1)

cube = vtkCubeSource()
cubeMapper = vtkPolyDataMapper()
cubeMapper.SetInput(cube.GetOutput())
cubeActor = vtkActor()
cubeActor.SetMapper(cubeMapper)
cubeActor.SetPosition(0.0,.25,0)
cubeActor.GetProperty().SetColor(0,0,1)

cone = vtkConeSource()
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)
coneActor.SetPosition(0,0,.25)
coneActor.GetProperty().SetColor(0,1,0)

cylinder = vtkCylinderSource()
cylinderMapper = vtkPolyDataMapper()
cylinderMapper.SetInput(cylinder.GetOutput())
cylinderActor = vtkAssembly()
cylinderActor.SetMapper(cylinderMapper)
cylinderActor.AddPart(sphereActor)
cylinderActor.AddPart(cubeActor)
cylinderActor.AddPart(coneActor)
cylinderActor.SetOrigin(5,10,15)
cylinderActor.AddPosition(5,0,0)
cylinderActor.RotateX(15)
cylinderActor.GetProperty().SetColor(1,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(cylinderActor)
ren.AddActor(coneActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)

# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()




iren.Start()
