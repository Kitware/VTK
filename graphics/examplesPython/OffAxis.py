#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
ren2 = vtkRenderer()
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a sphere source and actor
#
sphere = vtkSphereSource()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInput(sphere.GetOutput())
sphereActor = vtkActor()
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
spikeActor = vtkActor()
spikeActor.SetMapper(spikeMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(sphereActor)
ren.AddActor(spikeActor)
ren.SetBackground(0.1,0.2,0.4)
ren.SetViewport(0,0,0.5,1)
ren2.AddActor(sphereActor)
ren2.AddActor(spikeActor)
ren2.SetBackground(0.1,0.4,0.2)
ren2.SetViewport(0.5,0,1,1)
renWin.SetSize(500,500)

# render the image
#
iren.Initialize()
cam1=ren.GetActiveCamera()
cam2=ren2.GetActiveCamera()
cam1.SetWindowCenter(-1.01,0)
cam2.SetWindowCenter(1.01,0)

renWin.Render()
iren.Start()
