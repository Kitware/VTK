#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Mace example

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a sphere source and actor
#
sphere = vtkSphereSource()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInput(sphere.GetOutput())
sphereActor = vtkLODActor()
sphereActor.SetMapper(sphereMapper)

# create the spikes using a cone source and the sphere source
#
cone = vtkConeSource()
glyph = vtkGlyph3D()
glyph.SetInput(sphere.GetOutput())
glyph.SetSource(cone.GetOutput())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)
spikeMapper = vtkPolyDataMapper()
spikeMapper.SetInput(glyph.GetOutput())
spikeActor = vtkLODActor()
spikeActor.SetMapper(spikeMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(sphereActor)
ren.AddActor(spikeActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)

# render the image
#
cam1=ren.GetActiveCamera()
cam1.ParallelProjectionOn()
cam1.SetParallelScale(1)
cam1.Zoom(1.4)
iren.Initialize()

iren.Start()
