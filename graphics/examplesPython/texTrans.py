#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

# Test the texture transformation object

# Get the interactor

# load in the texture map
#
pnmReader = vtkPNMReader()
pnmReader.SetFileName(VTK_DATA + "/masonry.ppm")
atext = vtkTexture()
atext.SetInput(pnmReader.GetOutput())
atext.InterpolateOn()

# create a plane source and actor
plane = vtkPlaneSource()
trans = vtkTransformTextureCoords()
trans.SetInput(plane.GetOutput())
trans.SetScale(2,3,1)
trans.FlipSOn()
trans.SetPosition(0.5,1.0,0.0) #need.to.do.this.because.of.non-zero.origin
planeMapper = vtkDataSetMapper()
planeMapper.SetInput(trans.GetOutput())
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.SetTexture(atext)

# Create graphics stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(planeActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)
renWin.Render()







iren.Start()
