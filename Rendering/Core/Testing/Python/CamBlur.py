#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create the piplinee, ball and spikes
sphere = vtk.vtkSphereSource()
sphere.SetThetaResolution(7)
sphere.SetPhiResolution(7)
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor2 = vtk.vtkActor()
sphereActor2.SetMapper(sphereMapper)
cone = vtk.vtkConeSource()
cone.SetResolution(5)
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)
spikeMapper = vtk.vtkPolyDataMapper()
spikeMapper.SetInputConnection(glyph.GetOutputPort())
spikeActor = vtk.vtkActor()
spikeActor.SetMapper(spikeMapper)
spikeActor2 = vtk.vtkActor()
spikeActor2.SetMapper(spikeMapper)
# set the actors position and scale
spikeActor.SetPosition(0,0.7,0)
sphereActor.SetPosition(0,0.7,0)
spikeActor2.SetPosition(0,-1,-10)
sphereActor2.SetPosition(0,-1,-10)
spikeActor2.SetScale(1.5,1.5,1.5)
sphereActor2.SetScale(1.5,1.5,1.5)
ren1.AddActor(sphereActor)
ren1.AddActor(spikeActor)
ren1.AddActor(sphereActor2)
ren1.AddActor(spikeActor2)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(200,200)
# do the first render and then zoom in a little
renWin.Render()
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().Zoom(1.8)
ren1.GetActiveCamera().SetFocalDisk(0.05)
renWin.SetFDFrames(11)
renWin.Render()
iren.Initialize()
#renWin SetFileName CamBlur.tcl.ppm
#renWin SaveImageAsPPM
# prevent the tk window from showing up then start the event loop
# --- end of script --
