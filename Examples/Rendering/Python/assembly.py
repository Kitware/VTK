#!/usr/bin/env python

# This example demonstrates the use of vtkAssembly.  In an assembly,
# the motion of one actor affects the position of other actors.

import vtk

# Create four parts: a top level assembly (in this case, a
# vtkCylinder) and three primitives (using vtkSphereSource,
# vtkCubeSource, and vtkConeSource).  Set up mappers and actors for
# each part of the assembly to carry information about material
# properties and associated geometry.
sphere = vtk.vtkSphereSource()
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInput(sphere.GetOutput())
sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.SetOrigin(2, 1, 3)
sphereActor.RotateY(6)
sphereActor.SetPosition(2.25, 0, 0)
sphereActor.GetProperty().SetColor(1, 0, 1)

cube = vtk.vtkCubeSource()
cubeMapper = vtk.vtkPolyDataMapper()
cubeMapper.SetInput(cube.GetOutput())
cubeActor = vtk.vtkActor()
cubeActor.SetMapper(cubeMapper)
cubeActor.SetPosition(0.0, .25, 0)
cubeActor.GetProperty().SetColor(0, 0, 1)

cone = vtk.vtkConeSource()
coneMapper = vtk.vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtk.vtkActor()
coneActor.SetMapper(coneMapper)
coneActor.SetPosition(0, 0, .25)
coneActor.GetProperty().SetColor(0, 1, 0)

# top part of the assembly
cylinder = vtk.vtkCylinderSource()
cylinderMapper = vtk.vtkPolyDataMapper()
cylinderMapper.SetInput(cylinder.GetOutput())
cylinderMapper.SetResolveCoincidentTopologyToPolygonOffset()
cylinderActor = vtk.vtkActor()
cylinderActor.SetMapper(cylinderMapper)
cylinderActor.GetProperty().SetColor(1, 0, 0)

# Create the assembly and add the 4 parts to it.  Also set the origin,
# position and orientation in space.
assembly = vtk.vtkAssembly()
assembly.AddPart(cylinderActor)
assembly.AddPart(sphereActor)
assembly.AddPart(cubeActor)
assembly.AddPart(coneActor)
assembly.SetOrigin(5, 10, 15)
assembly.AddPosition(5, 0, 0)
assembly.RotateX(15)

# Create the Renderer, RenderWindow, and RenderWindowInteractor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(assembly)
ren.AddActor(coneActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(200, 200)

# Set up the camera to get a particular view of the scene
camera = vtk.vtkCamera()
camera.SetClippingRange(21.9464, 30.0179)
camera.SetFocalPoint(3.49221, 2.28844, -0.970866)
camera.SetPosition(3.49221, 2.28844, 24.5216)
camera.SetViewAngle(30)
camera.SetViewUp(0, 1, 0)
ren.SetActiveCamera(camera)

iren.Initialize()
renWin.Render()
iren.Start()
