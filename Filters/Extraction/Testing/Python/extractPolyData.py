#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Demonstrate how to extract polygonal cells with an implicit function
# get the interactor ui
# create a sphere source and actor
#
sphere = vtk.vtkSphereSource()
sphere.SetThetaResolution(8)
sphere.SetPhiResolution(16)
sphere.SetRadius(1.5)
# Extraction stuff
t = vtk.vtkTransform()
t.RotateX(90)
cylfunc = vtk.vtkCylinder()
cylfunc.SetRadius(0.5)
cylfunc.SetTransform(t)
extract = vtk.vtkExtractPolyDataGeometry()
extract.SetInputConnection(sphere.GetOutputPort())
extract.SetImplicitFunction(cylfunc)
extract.ExtractBoundaryCellsOn()
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(extract.GetOutputPort())
sphereMapper.GlobalImmediateModeRenderingOn()
sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetWindowName("vtk - extractPolyData")
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(sphereActor)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
