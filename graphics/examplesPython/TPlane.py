#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a plane source and actor
plane = vtkPlaneSource()
planeMapper = vtkPolyDataMapper()
planeMapper.SetInput(plane.GetOutput())
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)


# load in the texture map
#
atext = vtkTexture()
pnmReader = vtkPNMReader()
pnmReader.SetFileName(VTK_DATA + "/masonry.ppm")
atext.SetInput(pnmReader.GetOutput())
atext.InterpolateOn()
planeActor.SetTexture(atext)

view = vtkImageViewer()
view.SetInput(pnmReader.GetOutput())
view.Render()

# Add the actors to the renderer, set the background and size
ren.AddActor(planeActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)

# render the image
iren.Initialize()
cam1=ren.GetActiveCamera()
cam1.Elevation(-30)
cam1.Roll(-20)
renWin.Render()

iren.Start()
