#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

#
# structured points to geometry
#


# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read in some structured points
#
reader = vtkPNMReader()
reader.SetFileName(VTK_DATA + "/B.pgm")

geometry = vtkStructuredPointsGeometryFilter()
geometry.SetInput(reader.GetOutput())
geometry.SetExtent(0,10000,0,10000,0,0)

warp = vtkWarpScalar()
warp.SetInput(geometry.GetOutput())
warp.SetScaleFactor(-.1)

mapper = vtkDataSetMapper()
mapper.SetInput(warp.GetOutput())
mapper.SetScalarRange(0,255)
mapper.ImmediateModeRenderingOff()

actor = vtkActor()
actor.SetMapper(mapper)


# Add the actors to the renderer, set the background and size
#
ren.AddActor(actor)
ren.GetActiveCamera().Azimuth(20)
ren.GetActiveCamera().Elevation(30)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(640,480)

# render the image
#
cam1=ren.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()



iren.Start()
