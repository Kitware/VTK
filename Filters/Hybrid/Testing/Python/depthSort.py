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
camera = vtk.vtkCamera()
ren1.SetActiveCamera(camera)
# create a sphere source and actor
#
sphere = vtk.vtkSphereSource()
sphere.SetThetaResolution(80)
sphere.SetPhiResolution(40)
sphere.SetRadius(1)
sphere.SetCenter(0,0,0)
sphere2 = vtk.vtkSphereSource()
sphere2.SetThetaResolution(80)
sphere2.SetPhiResolution(40)
sphere2.SetRadius(0.5)
sphere2.SetCenter(1,0,0)
sphere3 = vtk.vtkSphereSource()
sphere3.SetThetaResolution(80)
sphere3.SetPhiResolution(40)
sphere3.SetRadius(0.5)
sphere3.SetCenter(-1,0,0)
sphere4 = vtk.vtkSphereSource()
sphere4.SetThetaResolution(80)
sphere4.SetPhiResolution(40)
sphere4.SetRadius(0.5)
sphere4.SetCenter(0,1,0)
sphere5 = vtk.vtkSphereSource()
sphere5.SetThetaResolution(80)
sphere5.SetPhiResolution(40)
sphere5.SetRadius(0.5)
sphere5.SetCenter(0,-1,0)
appendData = vtk.vtkAppendPolyData()
appendData.AddInputConnection(sphere.GetOutputPort())
appendData.AddInputConnection(sphere2.GetOutputPort())
appendData.AddInputConnection(sphere3.GetOutputPort())
appendData.AddInputConnection(sphere4.GetOutputPort())
appendData.AddInputConnection(sphere5.GetOutputPort())
depthSort = vtk.vtkDepthSortPolyData()
depthSort.SetInputConnection(appendData.GetOutputPort())
depthSort.SetDirectionToBackToFront()
depthSort.SetVector(1,1,1)
depthSort.SetCamera(camera)
depthSort.SortScalarsOn()
depthSort.Update()
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(depthSort.GetOutputPort())
mapper.SetScalarRange(0,depthSort.GetOutput().GetNumberOfCells())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetOpacity(0.5)
actor.GetProperty().SetColor(1,0,0)
actor.RotateX(-72)
depthSort.SetProp3D(actor)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,200)
# render the image
#
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(2.2)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
