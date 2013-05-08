#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

sphere = vtk.vtkSphereSource()

# make two copies of the shape and distort them a little
transform1 = vtk.vtkTransform()
transform1.Translate(0.2, 0.1, 0.3)
transform1.Scale(1.3, 1.1, 0.8)

transform2 = vtk.vtkTransform()
transform2.Translate(0.3, 0.7, 0.1)
transform2.Scale(1.0, 0.1, 1.8)

transformer1 = vtk.vtkTransformPolyDataFilter()
transformer1.SetInputConnection(sphere.GetOutputPort())
transformer1.SetTransform(transform1)

transformer2 = vtk.vtkTransformPolyDataFilter()
transformer2.SetInputConnection(sphere.GetOutputPort())
transformer2.SetTransform(transform2)

# map these three shapes into the first renderer
map1a = vtk.vtkPolyDataMapper()
map1a.SetInputConnection(sphere.GetOutputPort())

Actor1a = vtk.vtkActor()
Actor1a.SetMapper(map1a)
Actor1a.GetProperty().SetDiffuseColor(1.0000, 0.3882, 0.2784)

map1b = vtk.vtkPolyDataMapper()
map1b.SetInputConnection(transformer1.GetOutputPort())

Actor1b = vtk.vtkActor()
Actor1b.SetMapper(map1b)
Actor1b.GetProperty().SetDiffuseColor(0.3882, 1.0000, 0.2784)

map1c = vtk.vtkPolyDataMapper()
map1c.SetInputConnection(transformer2.GetOutputPort())

Actor1c = vtk.vtkActor()
Actor1c.SetMapper(map1c)
Actor1c.GetProperty().SetDiffuseColor(0.3882, 0.2784, 1.0000)

group = vtk.vtkMultiBlockDataGroupFilter()
group.AddInputConnection(sphere.GetOutputPort())
group.AddInputConnection(transformer1.GetOutputPort())
group.AddInputConnection(transformer2.GetOutputPort())

# align the shapes using Procrustes (using SetModeToRigidBody)
procrustes1 = vtk.vtkProcrustesAlignmentFilter()
procrustes1.SetInputConnection(group.GetOutputPort())
procrustes1.GetLandmarkTransform().SetModeToRigidBody()
procrustes1.StartFromCentroidOn()
procrustes1.Update()

# map the aligned shapes into the second renderer
map2a = vtk.vtkPolyDataMapper()
map2a.SetInputData(procrustes1.GetOutput().GetBlock(0))

Actor2a = vtk.vtkActor()
Actor2a.SetMapper(map2a)
Actor2a.GetProperty().SetDiffuseColor(1.0000, 0.3882, 0.2784)

map2b = vtk.vtkPolyDataMapper()
map2b.SetInputData(procrustes1.GetOutput().GetBlock(1))

Actor2b = vtk.vtkActor()
Actor2b.SetMapper(map2b)
Actor2b.GetProperty().SetDiffuseColor(0.3882, 1.0000, 0.2784)

map2c = vtk.vtkPolyDataMapper()
map2c.SetInputData(procrustes1.GetOutput().GetBlock(2))

Actor2c = vtk.vtkActor()
Actor2c.SetMapper(map2c)
Actor2c.GetProperty().SetDiffuseColor(0.3882, 0.2784, 1.0000)

# align the shapes using Procrustes (using SetModeToSimilarity (default))
procrustes2 = vtk.vtkProcrustesAlignmentFilter()
procrustes2.SetInputConnection(group.GetOutputPort())
procrustes2.Update()

# map the aligned shapes into the third renderer
map3a = vtk.vtkPolyDataMapper()
map3a.SetInputData(procrustes2.GetOutput().GetBlock(0))

Actor3a = vtk.vtkActor()
Actor3a.SetMapper(map3a)
Actor3a.GetProperty().SetDiffuseColor(1.0000, 0.3882, 0.2784)

map3b = vtk.vtkPolyDataMapper()
map3b.SetInputData(procrustes2.GetOutput().GetBlock(1))

Actor3b = vtk.vtkActor()
Actor3b.SetMapper(map3b)
Actor3b.GetProperty().SetDiffuseColor(0.3882, 1.0000, 0.2784)

map3c = vtk.vtkPolyDataMapper()
map3c.SetInputData(procrustes2.GetOutput().GetBlock(2))

Actor3c = vtk.vtkActor()
Actor3c.SetMapper(map3c)
Actor3c.GetProperty().SetDiffuseColor(0.3882, 0.2784, 1.0000)

# Create the RenderWindow and its three Renderers
ren1 = vtk.vtkRenderer()
ren2 = vtk.vtkRenderer()
ren3 = vtk.vtkRenderer()

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)

renWin.SetSize(300, 100)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer
ren1.AddActor(Actor1a)
ren1.AddActor(Actor1b)
ren1.AddActor(Actor1c)
ren2.AddActor(Actor2a)
ren2.AddActor(Actor2b)
ren2.AddActor(Actor2c)
ren3.AddActor(Actor3a)
ren3.AddActor(Actor3b)
ren3.AddActor(Actor3c)

# set the properties of the renderers
ren1.SetBackground(1, 1, 1)
ren1.SetViewport(0.0, 0.0, 0.33, 1.0)
ren1.ResetCamera()
ren1.GetActiveCamera().SetPosition(1, -1, 0)
ren1.ResetCamera()
ren2.SetBackground(1, 1, 1)
ren2.SetViewport(0.33, 0.0, 0.66, 1.0)

# ren2.ResetCamera()
# ren2.GetActiveCamera().SetPosition(1. -1, 0)
# ren2.ResetCamera()
ren2.SetActiveCamera(ren1.GetActiveCamera())

ren3.SetBackground(1, 1, 1)
ren3.SetViewport(0.66, 0.0, 1.0, 1.0)
ren3.ResetCamera()
ren3.GetActiveCamera().SetPosition(1, -1, 0)
ren3.ResetCamera()

renWin.Render()
print Actor1b.GetCenter()
print Actor2b.GetCenter()
print Actor3b.GetCenter()

iren.Initialize()
#iren.Start()
