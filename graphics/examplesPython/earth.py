#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *
from libVTKContribPython import *

# Mace example

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

tss = vtkTexturedSphereSource()
tss.SetThetaResolution(18)
tss.SetPhiResolution(9)
earthMapper = vtkPolyDataMapper()
earthMapper.SetInput(tss.GetOutput())
earthActor = vtkActor()
earthActor.SetMapper(earthMapper)

# load in the texture map
#
atext = vtkTexture()
pnmReader = vtkPNMReader()
pnmReader.SetFileName(VTK_DATA + "/earth.ppm")
atext.SetInput(pnmReader.GetOutput())
atext.InterpolateOn()
earthActor.SetTexture(atext)

# create a earth source and actor
#
es = vtkEarthSource()
es.SetRadius(0.501)
es.SetOnRatio(2)
earth2Mapper = vtkPolyDataMapper()
earth2Mapper.SetInput(es.GetOutput())
earth2Actor = vtkActor()
earth2Actor.SetMapper(earth2Mapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(earthActor)
ren.AddActor(earth2Actor)
ren.SetBackground(0,0,0.1)
renWin.SetSize(300,300)

# render the image
#
cam1=ren.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()


# export to rib format
rib = vtkRIBExporter()
rib.SetFilePrefix("earth")
rib.SetRenderWindow(renWin)
rib.BackgroundOn()
rib.Write()

iren.Start()
