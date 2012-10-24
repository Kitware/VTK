#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# Demonstrate the use of clipping and capping on polyhedral data
#
# create a sphere and clip it
#
sphere = vtk.vtkSphereSource()
sphere.SetRadius(1)
sphere.SetPhiResolution(10)
sphere.SetThetaResolution(10)
plane1 = vtk.vtkPlane()
plane1.SetOrigin(0.3,0.3,0.3)
plane1.SetNormal(-1,-1,-1)
plane2 = vtk.vtkPlane()
plane2.SetOrigin(0.5,0,0)
plane2.SetNormal(-1,0,0)
planes = vtk.vtkPlaneCollection()
planes.AddItem(plane1)
planes.AddItem(plane2)
# stripper just increases coverage
stripper = vtk.vtkStripper()
stripper.SetInputConnection(sphere.GetOutputPort())
clipper = vtk.vtkClipClosedSurface()
clipper.SetInputConnection(stripper.GetOutputPort())
clipper.SetClippingPlanes(planes)
clipperOutline = vtk.vtkClipClosedSurface()
clipperOutline.SetInputConnection(stripper.GetOutputPort())
clipperOutline.SetClippingPlanes(planes)
clipperOutline.GenerateFacesOff()
clipperOutline.GenerateOutlineOn()
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(clipper.GetOutputPort())
clipperOutlineMapper = vtk.vtkPolyDataMapper()
clipperOutlineMapper.SetInputConnection(clipperOutline.GetOutputPort())
clipActor = vtk.vtkActor()
clipActor.SetMapper(sphereMapper)
clipActor.GetProperty().SetColor(0.8,0.05,0.2)
clipOutlineActor = vtk.vtkActor()
clipOutlineActor.SetMapper(clipperOutlineMapper)
clipOutlineActor.GetProperty().SetColor(0,1,0)
clipOutlineActor.SetPosition(0.001,0.001,0.001)
# create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sphere.GetOutputPort())
outline.GenerateFacesOn()
outlineClip = vtk.vtkClipClosedSurface()
outlineClip.SetClippingPlanes(planes)
outlineClip.SetInputConnection(outline.GetOutputPort())
outlineClip.GenerateFacesOff()
outlineClip.GenerateOutlineOn()
outlineClip.SetScalarModeToColors()
outlineClip.SetClipColor(0,1,0)
outlineMapper = vtk.vtkDataSetMapper()
outlineMapper.SetInputConnection(outlineClip.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.SetPosition(0.001,0.001,0.001)
# Create graphics stuff
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetMultiSamples(0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(clipOutlineActor)
ren1.AddActor(outlineActor)
ren1.SetBackground(1,1,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.GetActiveCamera().Dolly(1.2)
ren1.ResetCameraClippingRange()
renWin.SetSize(300,300)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
