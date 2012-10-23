#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create four parts: a top level assembly and three primitives
#
sphere = vtk.vtkSphereSource()
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.SetOrigin(2,1,3)
sphereActor.RotateY(6)
sphereActor.SetPosition(2.25,0,0)
sphereActor.GetProperty().SetColor(1,0,1)
cube = vtk.vtkCubeSource()
cubeMapper = vtk.vtkPolyDataMapper()
cubeMapper.SetInputConnection(cube.GetOutputPort())
cubeActor = vtk.vtkActor()
cubeActor.SetMapper(cubeMapper)
cubeActor.SetPosition(0.0,.25,0)
cubeActor.GetProperty().SetColor(0,0,1)
cone = vtk.vtkConeSource()
coneMapper = vtk.vtkPolyDataMapper()
coneMapper.SetInputConnection(cone.GetOutputPort())
coneActor = vtk.vtkActor()
coneActor.SetMapper(coneMapper)
coneActor.SetPosition(0,0,.25)
coneActor.GetProperty().SetColor(0,1,0)
cylinder = vtk.vtkCylinderSource()
#top part
cylinderMapper = vtk.vtkPolyDataMapper()
cylinderMapper.SetInputConnection(cylinder.GetOutputPort())
cylinderMapper.SetResolveCoincidentTopologyToPolygonOffset()
cylinderActor = vtk.vtkActor()
cylinderActor.SetMapper(cylinderMapper)
cylinderActor.GetProperty().SetColor(1,0,0)
assembly = vtk.vtkAssembly()
assembly.AddPart(cylinderActor)
assembly.AddPart(sphereActor)
assembly.AddPart(cubeActor)
assembly.AddPart(coneActor)
assembly.SetOrigin(5,10,15)
assembly.AddPosition(5,0,0)
assembly.RotateX(15)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(assembly)
ren1.AddActor(coneActor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(200,200)
# Get handles to some useful objects
#
camera = vtk.vtkCamera()
camera.SetClippingRange(21.9464,30.0179)
camera.SetFocalPoint(3.49221,2.28844,-0.970866)
camera.SetPosition(3.49221,2.28844,24.5216)
camera.SetViewAngle(30)
camera.SetViewUp(0,1,0)
ren1.SetActiveCamera(camera)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
