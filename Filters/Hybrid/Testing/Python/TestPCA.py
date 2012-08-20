#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This example shows how to visualise the variation in shape in a set of objects using
# vtkPCAAnalysisFilter.
#
# We make three ellipsoids by distorting and translating a sphere and then align them together
# using vtkProcrustesAlignmentFilter, and then pass the output to vtkPCAAnalysisFilter. We visualise
# the first and second modes - the major sources of variation that were in the training set.
sphere = vtk.vtkSphereSource()
sphere.SetPhiResolution(36)
sphere.SetThetaResolution(36)
sphere.Update()
# make two copies of the shape and distort them a little
transform1 = vtk.vtkTransform()
transform1.Translate(0.2,0.1,0.3)
transform1.Scale(1.3,1.1,0.8)
transform2 = vtk.vtkTransform()
transform2.Translate(0.3,0.7,0.1)
transform2.Scale(1.0,0.1,1.8)
transformer1 = vtk.vtkTransformPolyDataFilter()
transformer1.SetInputConnection(sphere.GetOutputPort())
transformer1.SetTransform(transform1)
transformer1.Update()
transformer2 = vtk.vtkTransformPolyDataFilter()
transformer2.SetInputConnection(sphere.GetOutputPort())
transformer2.SetTransform(transform2)
transformer2.Update()
#------------------------------------------------------------------
# map these three shapes into the first renderer
#------------------------------------------------------------------
map1a = vtk.vtkPolyDataMapper()
map1a.SetInputConnection(sphere.GetOutputPort())
Actor1a = vtk.vtkActor()
Actor1a.SetMapper(map1a)
Actor1a.GetProperty().SetDiffuseColor(1.0000,0.3882,0.2784)
map1b = vtk.vtkPolyDataMapper()
map1b.SetInputConnection(transformer1.GetOutputPort())
Actor1b = vtk.vtkActor()
Actor1b.SetMapper(map1b)
Actor1b.GetProperty().SetDiffuseColor(0.3882,1.0000,0.2784)
map1c = vtk.vtkPolyDataMapper()
map1c.SetInputConnection(transformer2.GetOutputPort())
Actor1c = vtk.vtkActor()
Actor1c.SetMapper(map1c)
Actor1c.GetProperty().SetDiffuseColor(0.3882,0.2784,1.0000)
#------------------------------------------------------------------
# align the shapes using Procrustes (using SetModeToRigidBody)
# and map the aligned shapes into the second renderer
#------------------------------------------------------------------
group = vtk.vtkMultiBlockDataGroupFilter()
group.AddInputConnection(sphere.GetOutputPort())
group.AddInputConnection(transformer1.GetOutputPort())
group.AddInputConnection(transformer2.GetOutputPort())
procrustes = vtk.vtkProcrustesAlignmentFilter()
procrustes.SetInputConnection(group.GetOutputPort())
procrustes.GetLandmarkTransform().SetModeToRigidBody()
procrustes.Update()
map2a = vtk.vtkPolyDataMapper()
map2a.SetInputData(procrustes.GetOutput().GetBlock(0))
Actor2a = vtk.vtkActor()
Actor2a.SetMapper(map2a)
Actor2a.GetProperty().SetDiffuseColor(1.0000,0.3882,0.2784)
map2b = vtk.vtkPolyDataMapper()
map2b.SetInputData(procrustes.GetOutput().GetBlock(1))
Actor2b = vtk.vtkActor()
Actor2b.SetMapper(map2b)
Actor2b.GetProperty().SetDiffuseColor(0.3882,1.0000,0.2784)
map2c = vtk.vtkPolyDataMapper()
map2c.SetInputData(procrustes.GetOutput().GetBlock(2))
Actor2c = vtk.vtkActor()
Actor2c.SetMapper(map2c)
Actor2c.GetProperty().SetDiffuseColor(0.3882,0.2784,1.0000)
#------------------------------------------------------------------
# pass the output of Procrustes to vtkPCAAnalysisFilter
#------------------------------------------------------------------
pca = vtk.vtkPCAAnalysisFilter()
pca.SetInputConnection(procrustes.GetOutputPort())
pca.Update()
# we need to call Update because GetParameterisedShape is not
# part of the normal SetInput/GetOutput pipeline
#------------------------------------------------------------------
# map the first mode into the third renderer:
# -3,0,3 standard deviations on the first mode
# illustrate the extremes around the average shape
#------------------------------------------------------------------
params = vtk.vtkFloatArray()
params.SetNumberOfComponents(1)
params.SetNumberOfTuples(1)
params.SetTuple1(0,0.0)
shapea = vtk.vtkPolyData()
shapea.DeepCopy(sphere.GetOutput())
pca.GetParameterisedShape(params,shapea)
normalsa = vtk.vtkPolyDataNormals()
normalsa.SetInputData(shapea)
map3a = vtk.vtkPolyDataMapper()
map3a.SetInputConnection(normalsa.GetOutputPort())
Actor3a = vtk.vtkActor()
Actor3a.SetMapper(map3a)
Actor3a.GetProperty().SetDiffuseColor(1,1,1)
params.SetTuple1(0,-3.0)
shapeb = vtk.vtkPolyData()
shapeb.DeepCopy(sphere.GetOutput())
pca.GetParameterisedShape(params,shapeb)
normalsb = vtk.vtkPolyDataNormals()
normalsb.SetInputData(shapeb)
map3b = vtk.vtkPolyDataMapper()
map3b.SetInputConnection(normalsb.GetOutputPort())
Actor3b = vtk.vtkActor()
Actor3b.SetMapper(map3b)
Actor3b.GetProperty().SetDiffuseColor(1,1,1)
params.SetTuple1(0,3.0)
shapec = vtk.vtkPolyData()
shapec.DeepCopy(sphere.GetOutput())
pca.GetParameterisedShape(params,shapec)
normalsc = vtk.vtkPolyDataNormals()
normalsc.SetInputData(shapec)
map3c = vtk.vtkPolyDataMapper()
map3c.SetInputConnection(normalsc.GetOutputPort())
Actor3c = vtk.vtkActor()
Actor3c.SetMapper(map3c)
Actor3c.GetProperty().SetDiffuseColor(1,1,1)
#------------------------------------------------------------------
# map the second mode into the fourth renderer:
#------------------------------------------------------------------
params4 = vtk.vtkFloatArray()
params4.SetNumberOfComponents(1)
params4.SetNumberOfTuples(2)
params4.SetTuple1(0,0.0)
params4.SetTuple1(1,-3.0)
shape4a = vtk.vtkPolyData()
shape4a.DeepCopy(sphere.GetOutput())
pca.GetParameterisedShape(params4,shape4a)
normals4a = vtk.vtkPolyDataNormals()
normals4a.SetInputData(shape4a)
map4a = vtk.vtkPolyDataMapper()
map4a.SetInputConnection(normals4a.GetOutputPort())
Actor4a = vtk.vtkActor()
Actor4a.SetMapper(map4a)
Actor4a.GetProperty().SetDiffuseColor(1,1,1)
params4.SetTuple1(1,0.0)
shape4b = vtk.vtkPolyData()
shape4b.DeepCopy(sphere.GetOutput())
pca.GetParameterisedShape(params4,shape4b)
normals4b = vtk.vtkPolyDataNormals()
normals4b.SetInputData(shape4b)
map4b = vtk.vtkPolyDataMapper()
map4b.SetInputConnection(normals4b.GetOutputPort())
Actor4b = vtk.vtkActor()
Actor4b.SetMapper(map4b)
Actor4b.GetProperty().SetDiffuseColor(1,1,1)
params4.SetTuple1(1,3.0)
shape4c = vtk.vtkPolyData()
shape4c.DeepCopy(sphere.GetOutput())
pca.GetParameterisedShape(params4,shape4c)
normals4c = vtk.vtkPolyDataNormals()
normals4c.SetInputData(shape4c)
map4c = vtk.vtkPolyDataMapper()
map4c.SetInputConnection(normals4c.GetOutputPort())
Actor4c = vtk.vtkActor()
Actor4c.SetMapper(map4c)
Actor4c.GetProperty().SetDiffuseColor(1,1,1)
#------------------------------------------------------------------
# Create the RenderWindow and its four Renderers
#------------------------------------------------------------------
ren1 = vtk.vtkRenderer()
ren2 = vtk.vtkRenderer()
ren3 = vtk.vtkRenderer()
ren4 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
renWin.SetSize(600,200)
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
ren4.AddActor(Actor4a)
ren4.AddActor(Actor4b)
ren4.AddActor(Actor4c)
# set the properties of the renderers
ren1.SetBackground(1,1,1)
ren1.SetViewport(0.0,0.0,0.25,1.0)
ren1.ResetCamera()
ren1.GetActiveCamera().SetPosition(1,-1,0)
ren1.ResetCamera()
ren2.SetBackground(1,1,1)
ren2.SetViewport(0.25,0.0,0.5,1.0)
ren2.ResetCamera()
ren2.GetActiveCamera().SetPosition(1,-1,0)
ren2.ResetCamera()
ren3.SetBackground(1,1,1)
ren3.SetViewport(0.5,0.0,0.75,1.0)
ren3.ResetCamera()
ren3.GetActiveCamera().SetPosition(1,-1,0)
ren3.ResetCamera()
ren4.SetBackground(1,1,1)
ren4.SetViewport(0.75,0.0,1.0,1.0)
ren4.ResetCamera()
ren4.GetActiveCamera().SetPosition(1,-1,0)
ren4.ResetCamera()
# render the image
#
renWin.Render()
# output the image to file (used to generate the initial regression image)
#vtkWindowToImageFilter to_image
#to_image SetInput renWin
#vtkPNGWriter to_png
#to_png SetFileName "TestPCA.png"
#to_png SetInputConnection [to_image GetOutputPort]
#to_png Write
# prevent the tk window from showing up then start the event loop
# --- end of script --
