#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# this demonstrates assemblies hierarchies
# include get the vtk interactor ui

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

sphere = vtkSphereSource()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInput(sphere.GetOutput())
sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.SetOrigin(2,1,3)
sphereActor.RotateY(6)
sphereActor.SetPosition(2.25,0,0)
sphereActor.GetProperty().SetColor(1,1,0)

cube = vtkCubeSource()
cubeMapper = vtkPolyDataMapper()
cubeMapper.SetInput(cube.GetOutput())
cubeActor = vtkActor()
cubeActor.SetMapper(cubeMapper)
cubeActor.SetPosition(0.0,.25,0)
cubeActor.GetProperty().SetColor(0,1,1)

cone = vtkConeSource()
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)
coneActor.SetPosition(0,0,.25)
coneActor.GetProperty().SetColor(1,0,1)

cylinder = vtkCylinderSource()
cylinderMapper = vtkPolyDataMapper()
cylinderMapper.SetInput(cylinder.GetOutput())
cylActor = vtkActor()
cylActor.SetMapper(cylinderMapper)
cylinderActor = vtkAssembly()
cylinderActor.SetMapper(cylinderMapper)
cylinderActor.AddPart(sphereActor)
cylinderActor.AddPart(cubeActor)
cylinderActor.AddPart(coneActor)
cylinderActor.SetOrigin(5,10,15)
cylinderActor.AddPosition(5,0,0)
cylinderActor.RotateX(45)
cylinderActor.GetProperty().SetColor(1,0,0)

cylinderActor2 = vtkAssembly()
cylinderActor2.SetMapper(cylinderMapper)
cylinderActor2.AddPart(sphereActor)
cylinderActor2.AddPart(cubeActor)
cylinderActor2.AddPart(coneActor)
cylinderActor2.SetOrigin(5,10,15)
cylinderActor2.AddPosition(6,0,0)
cylinderActor2.RotateX(50)
cylinderActor2.GetProperty().SetColor(0,1,0)

twoGroups = vtkAssembly()
twoGroups.AddPart(cylinderActor)
twoGroups.AddPart(cylinderActor2)
twoGroups.AddPosition(0,0,2)
twoGroups.RotateX(15)
    
twoGroups2 = vtkAssembly()
twoGroups2.AddPart(cylinderActor)
twoGroups2.AddPart(cylinderActor2)
twoGroups2.AddPosition(3,0,0)


twoGroups3 = vtkAssembly()
twoGroups3.AddPart(cylinderActor)
twoGroups3.AddPart(cylinderActor2)
twoGroups3.AddPosition(0,4,0)

threeGroups = vtkAssembly()
threeGroups.AddPart(twoGroups)
threeGroups.AddPart(twoGroups2)
threeGroups.AddPart(twoGroups3)

threeGroups2 = vtkAssembly()
threeGroups2.AddPart(twoGroups)
threeGroups2.AddPart(twoGroups2)
threeGroups2.AddPart(twoGroups3)
threeGroups2.AddPosition(5,5,5)

topLevel = vtkAssembly()
topLevel.AddPart(threeGroups)
topLevel.AddPart(threeGroups2)

# Add the actors to the renderer, set the background and size
#

ren.AddActor(threeGroups)
ren.AddActor(threeGroups2)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)

# Get handles to some useful objects
#
iren.Initialize()



iren.Start()
